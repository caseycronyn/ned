#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/history.h>
#include <readline/readline.h>

#include "ed.h"

bool is_terminator(char *line);

void initialize_readline(void);

char *command_generator(const char *, int);

char **ed_completion(const char *, int, int);

char *add_readline_line(const char *sb, const char *new_line);

void free_candidates(void);

int done;
extern int newline_added;

// char *commands[] = {"cd",  "delete", "help",   "?",    "list", "ls", "pwd", "quit",   "rename", "stat", "view", NULL};

char **completion_candidates = NULL;
int completion_count = 0;

void free_candidates(void) {
     if (!completion_candidates) {
	  return;
     }
     for (int i = 0; i < completion_count; ++i) {
	  free(completion_candidates[i]);
     }
     free(completion_candidates);
     completion_candidates = NULL;
     completion_count = 0;
}

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
unsigned long get_readline_line(void) {
     // Using this to check I'm not defaulting back to libedit
     // printf("%s\n", rl_library_version);
     char *line;

     initialize_readline();

     line = readline(NULL);
     // printf("%lu", strlen(line));
     unsigned long length = strlen(line);
     REALLOC(ibuf, ibufsz, length + 2, ERR);
     memcpy(ibuf, line, length);
     ibuf[length] = '\n';
     ibuf[length + 1] = '\0';

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
     // printf("text: %s\n", rl_line_buffer);
     // printf("cursor position: %d\n", rl_point);
     update_document(rl_line_buffer, rl_point);
     document_change(&doc, ser.to_server_fd);

     free_candidates();
     completion_response response = completion(&doc, &ser);
     completion_count = response.completion_count;
     completion_candidates = response.completion_items;

     // increase completion limit
     rl_completion_query_items = 1000;
     // remove default completion functionality (filenames)
     rl_attempted_completion_over = 1;
     return rl_completion_matches(text, command_generator);
}

char *command_generator(const char *text, int state) {
     static int list_index, len;
     char *name;

     // first pass
     if (!state) {
	  list_index = 0;
	  len = (int)strlen(text);
     }

     while (list_index < completion_count) {
	  name = completion_candidates[list_index++];
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
     // printf("%s", doc.text);
}

// modified version of display_lines()
char *get_temp_scratch_buffer(void) {
     char *s = NULL;
     char *buffer = NULL;
     int buf_length = 0;
     int buf_size = 0;
     int diff = -1;
     size_t growth = 0;

     line_t *bp = get_addressed_line_node(1);
     line_t *ep = get_addressed_line_node(INC_MOD(addr_last, addr_last));

     for (; bp != ep; bp = bp->q_forw) {
	  if ((s = get_sbuf_line(bp)) == NULL) {
	       free(buffer);
	       return NULL;
	  }

	  size_t line_length = (size_t)bp->len;
	  growth = buf_length + line_length + 2;
	  if (growth > buf_size) {
	       REALLOC(buffer, buf_size, growth, NULL);
	  }

	  memcpy(buffer + buf_length, s, line_length);
	  buf_length += line_length;
	  buffer[buf_length++] = '\n';
     }
     if (buf_size == 0) {
	  growth = 1;
	  REALLOC(buffer, buf_size, growth, NULL);
     }

     buffer[buf_length] = '\0';
     return buffer;
}

char *add_readline_line(const char *sb, const char *new_line) {
     if (!sb || !new_line) {
	  return NULL;
     }

     const size_t scratch_length = strlen(sb);
     const size_t newline_length = strlen(new_line);

     size_t offset = 0;
     int line_no = 0;
     while (line_no < current_addr && offset < scratch_length) {
	  if (sb[offset] == '\n') {
	       line_no++;
	  }
	  offset++;
     }

     bool newline_ending =
	  (newline_length > 0 && new_line[newline_length - 1] == '\n');
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

