#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "ed.h"

char *add_readline_line(char *sb, char *new_line);
bool is_terminator(char *line);
void initialize_readline(void);
char *command_generator(const char *, int);
char **ed_completion(const char *, int, int);

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

// TODO
// Here I want to maintain a 'complete context' for the lsp. This means the full context of the buffer + the current line
void update_document(char *cur_line, int cursor_offset) {
     char *scratch_buf = get_temp_scratch_buffer();
     printf("%s", add_readline_line(scratch_buf, cur_line));

     // placeholder function... go find changes to text document and update the document struct
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

char *add_readline_line(char *sb, char *new_line) {
     ssize_t read;
     char * line;
     char buf_sz = strlen(sb);
     char newln_sz = strlen(new_line);
     char *new_str = malloc(buf_sz + newln_sz + 1);
     FILE *fp = fmemopen(sb, buf_sz, "r");

     bool added = false;
     bool added_pass = false;
     int i = 0;
     int line_num = 1;
     while (fgets(line, buf_sz, fp) != NULL) {
	  if (!added && current_addr == line_num) {
	       added = true;
	       added_pass = true;
	       line = new_line;
	  }
	  new_str[i] = *line;
	  new_str[strlen(line) + 1] = '\n';
	  i += strlen(line) + 1;
	  if (added_pass) {
	       added_pass = false;
	       i-= strlen(line) + 1;
	  }
     }
     fclose(fp);
     return new_str;
}
