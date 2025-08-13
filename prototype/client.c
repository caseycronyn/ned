#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "../cJSON.h"

void start_server(int *to_server_fd, int *to_client_fd);
void print_cJSON(cJSON *obj);
cJSON *deserialize_json(char *file_name);
int get_id(void);
cJSON *make_request(char *method);

long ID = 1;

int main(void) {
     int to_server_fildes[2];
     int to_client_fildes[2];
     start_server(to_server_fildes, to_client_fildes);

     // parent process (client)
     cJSON *request = make_request("initialize");

     // write message to server
     char *payload = cJSON_PrintUnformatted(request);
     write(to_server_fildes[1], payload, strlen(payload));
     

     // wait for child process to finish
     wait(NULL);
     return 0;
}

// creates a cJSON request of the specified method type
cJSON *make_request(char *method) {
     cJSON *request = cJSON_CreateObject();
     assert(request);

     cJSON *jsonrpc_ob = cJSON_CreateString("2.0");
     cJSON_AddItemToObject(request, "jsonrpc", jsonrpc_ob);

     cJSON *id_ob = cJSON_CreateNumber(get_id());
     cJSON_AddItemToObject(request, "id", id_ob);

     cJSON *method_ob = cJSON_CreateString(method);
     cJSON_AddItemToObject(request, "method", method_ob);

     cJSON *params_ob = cJSON_CreateObject();
     cJSON_AddItemToObject(request, "params", params_ob);

     cJSON *processId_ob = cJSON_CreateNumber(getpid());
     cJSON_AddItemToObject(params_ob, "processId", processId_ob);

     cJSON *uri_ob = cJSON_CreateString(realpath("example-file.c", NULL));
     cJSON_AddItemToObject(params_ob, "rootUri", uri_ob);

     cJSON *capabilities_ob = cJSON_CreateObject();
     cJSON_AddItemToObject(params_ob, "capabilities", capabilities_ob);

     return request;
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



void print_cJSON(cJSON *obj) {
     printf("%s\n", cJSON_Print(obj));
}

cJSON *deserialize_json(char *file_name) {
     FILE *fp = fopen("json/requests/initialize-basic.json", "r");
     assert(fp);
     // get size of buffer
     fseek(fp, 0, SEEK_END);
     long buffer_size = ftell(fp);
     rewind(fp);

     char *buffer = malloc(buffer_size + 1);
     fread(buffer, buffer_size, 1, fp);
     buffer[buffer_size] = '\0';
     fclose(fp);
     // printf("%s", buffer);

     cJSON *initialize_json = cJSON_Parse(buffer);
     free(buffer);
     assert(initialize_json != NULL);
     return initialize_json;
}

// return and increment id
int get_id(void) {
     return ID++;
}
