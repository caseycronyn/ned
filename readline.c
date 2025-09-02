#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/history.h>
#include <readline/readline.h>

#include "ed.h"

bool is_terminator(const char *line);
void initialize_readline(void);
char *command_generator(const char *, int);
char **ed_completion(const char *, int, int);
char *add_readline_line(const char *sb, const char *new_line);
void free_candidates(void);

int done;
extern int newline_added;

void free_candidates(void) {
     if (!comp.items) {
	  return;
     }
     for (int i = 0; i < comp.count; ++i) {
	  free(comp.items[i]);
     }
     free(comp.items);
     comp.items = NULL;
     comp.count = 0;
}

/* get_readline_line: read a line of text using readline; return line length */
unsigned long get_readline_line(void) {
     initialize_readline();

     char *line = readline(NULL);
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
     update_document(rl_line_buffer, rl_point);
     document_change(&doc, ser.to_server_fd);

     free_candidates();
     comp = complete(&doc, &ser);

     rl_completion_query_items = 1000;
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

     while (list_index < comp.count) {
	  name = comp.items[list_index++];
	  if (strncmp(name, text, len) == 0) {
	       return (strdup(name));
	  }
     }

     // If no names matched, then return NULL.
     return NULL;
}

// returns a 2d array of completion items from the language server
completion get_completion_items(const char *response) {
     cJSON *obj = cJSON_Parse(response);

     cJSON *result = cJSON_GetObjectItem(obj, "result");
     cJSON *items = cJSON_GetObjectItem(result, "items");

     int arr_length = cJSON_GetArraySize(items);
     comp.items = malloc(arr_length * sizeof(char *));

     cJSON *iterator = NULL;
     int i = 0;
     cJSON_ArrayForEach(iterator, items) {
          cJSON *textEdit = cJSON_GetObjectItem(iterator, "textEdit");
          cJSON *newText = cJSON_GetObjectItem(textEdit, "newText");

          char *item = newText->valuestring;
          int length = strlen(item);

          comp.items[i] = malloc(length + 1);
          strlcpy(comp.items[i], item, length + 1);
          i++;
     }
     comp.count = i;
     cJSON_Delete(obj);
     return comp;
}

bool is_terminator(const char *line) {
     return ((strlen(line) == 1) && line[0] == '.');
}

void update_document(const char *cur_line, int cursor_offset) {
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
     size_t buf_length = 0;
     int buf_size = 0;
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

