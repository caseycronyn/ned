#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

bool is_terminator(char *line);
void initialize_readline(void);
char *command_generator(const char *, int);
char **fileman_completion(const char *, int, int);

int done;

char *commands[] = {"cd", "delete", "help", "?", "list", "ls", "pwd", "quit", "rename", "stat", "view", NULL};

int main(int argc, char **argv) {
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

void initialize_readline(void) {
     // Allow conditional parsing of the ~/.inputrc file.
     rl_readline_name = "ed";

     // Tell the completer that we want a crack first.
     rl_attempted_completion_function = fileman_completion;
}

char **fileman_completion(const char *text, int start, int end) {
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
