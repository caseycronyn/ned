#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "ed.h"

bool is_terminator(char *line);
void initialize_readline(void);
char *command_generator(const char *, int);
char **ed_completion(const char *, int, int);
char *add_readline_line(const char *sb, const char *new_line);

int done;
extern int newline_added;

char *commands[] = {"cd", "delete", "help", "?", "list", "ls", "pwd", "quit", "rename", "stat", "view", NULL};

int main_readline(void) {
     // tells me what readline version I'm using
     printf("%s\n", rl_library_version);  

     char *line;

     initialize_readline();

     while (done == 0) {
          line = readline("> ");

          if (!line)
               break;

          if (is_terminator(line)) {
               // should write history to '.history'
               write_history(".history");
               break;
          }

          if (*line) {
               add_history(line);
          }


          free(line);
     }
     return 0;
}

/* get_readline_line: read a line of text from readline; return line length */
int get_readline_line(void) {
     // Using this to check I'm not defaulting back to libedit
     // printf("%s\n", rl_library_version);  
     char *line;

     initialize_readline();

     line = readline(NULL);
     // printf("%lu", strlen(line));
     int length = strlen(line);
     REALLOC(ibuf, ibufsz, length + 2, ERR);
     memcpy(ibuf, line, length);
     ibuf[length]   = '\n';
     ibuf[length+1] = '\0';

     lineno++;
     ibufp = ibuf;
     free(line);
     return length + 1;
}


void initialize_readline(void) {
     // Allow conditional parsing of the ~/.inputrc file.
     rl_readline_name = "ed";

     // Tell the completer that we want a crack first.
     rl_attempted_completion_function = ed_completion;
}

char **ed_completion(const char *text, int start, int end) {
     // disable built in autocompletion (filenames)
     // printf("text: %s\n", rl_line_buffer);
     // printf("cursor position: %d\n", rl_point);
     update_document(rl_line_buffer, rl_point);
     rl_attempted_completion_over = 1;
     return rl_completion_matches(text, command_generator);
}

char *command_generator(const char *text, int state) {
     static int list_index, len;
     char *name;

     // first pass
     if (!state) {
          list_index = 0;
          len = strlen(text);
     }

     while ((name = commands[list_index++]) != NULL) {
          if (strncmp(name, text, len) == 0) {
               return (strdup(name));
          }      
     }

     // If no names matched, then return NULL.
     return NULL;
}

bool is_terminator(char *line) {
     return ((strlen(line) == 1) && line[0] == '.');
}

void update_document(char *cur_line, int cursor_offset) {
     char *scratch_buf = get_temp_scratch_buffer();
     doc.text = add_readline_line(scratch_buf, cur_line);
     doc.line = current_addr;
     doc.column = cursor_offset;
     new_version(&doc);
}

// modified version of display_lines()
char *get_temp_scratch_buffer(void) {
	char *s = NULL;
	char *buffer = NULL;
	int buf_length = 0;
	int buf_size = 0;

	line_t *bp = get_addressed_line_node(1);
	line_t *ep = get_addressed_line_node(INC_MOD(addr_last, addr_last));

	for (; bp != ep; bp = bp->q_forw) {
	     if ((s = get_sbuf_line(bp)) == NULL) {
		  return NULL;
	     }

	     int line_length = bp->len + 1;
	     int growth = buf_length + line_length;
	     if (growth > buf_size) {
		  int diff = growth - buf_size;
		  REALLOC(buffer, buf_size, diff, NULL);
		  
	     }

	     memcpy(buffer + buf_length, s, bp->len);
	     buf_length += bp->len;
	     buffer[buf_length++] = '\n';
	}
	buffer[buf_size] = '\0';
	return buffer;
}

char *add_readline_line(const char *sb, const char *new_line) {
    if (!sb || !new_line) {
	 return NULL;
    }

    size_t scratch_length = strlen(sb);
    size_t newline_length = strlen(new_line);

    size_t offset = 0;
    int line_no = 1;
    while (line_no < current_addr && offset < scratch_length) {
        if (sb[offset] == '\n') {
            line_no++;
        }
        offset++;
    }

    bool newline_ending = (newline_length > 0 && new_line[newline_length - 1] == '\n');
    size_t insert_length = newline_length;
    if (!newline_ending) {
	 insert_length++;
    }
    char *result = malloc(scratch_length + insert_length + 1);
    assert(result);

    // up to the added line
    memcpy(result, sb, offset);

    // added line
    char *newline_dst = result + offset;
    memcpy(newline_dst, new_line, newline_length);
    if (!newline_ending) {
        result[offset + newline_length] = '\n';
    }

    // rest of the buffer
    char *end_dst = result + offset + insert_length;
    const char *end_src = sb + offset;
    size_t end_len = scratch_length - offset;
    memcpy(end_dst, end_src, end_len);

    result[scratch_length + insert_length] = '\0';
    return result;
}
