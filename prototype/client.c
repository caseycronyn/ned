#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void start_server(int *to_server_fd, int *to_client_fd);

int main(void) {
     int to_server_fildes[2];
     int to_client_fildes[2];
     start_server(to_server_fildes, to_client_fildes);
     // parent process (client)
     
     // wait for child process to finish
     wait(NULL);
     return 0;
}



void start_server(int *to_server_fd, int *to_client_fd) {
     pipe(to_server_fd);
     pipe(to_client_fd);

     // child process (server)
     if (!fork()) {
	  // set read and write to stdin and stdout
	  close(0);
	  dup(to_server_fd[0]);
	  close(1);
	  dup(to_client_fd[1]);
	  execlp("clangd", "clangd", NULL);
     }
}
