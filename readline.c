#include <stdbool.h>
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

int done;

char *commands[] = {"cd", "delete", "help", "?", "list", "ls", "pwd", "quit", "rename", "stat", "view", NULL};

int readline_handler(void) {
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
