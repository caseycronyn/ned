#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "../cJSON.h"

// NOTE change the functions so they only pass by reference when they need to
// also need to add proper error handling here
typedef struct {
     char *file_name;
     char *uri;
     char *language;
     long version;
     char *text;
     long line;
     long column;
} document;

typedef struct {
     long ID;
     int to_server_fd[2];
     int to_client_fd[2];
} server;

void init_file(char *name);
void update_document_text(void);
void start_server(int *to_server_fd, int *to_client_fd);
void halt(server *s);
int get_id(server *s);
void send_message(int fd, cJSON *msg);
char *wait_for_response(int fd);
ssize_t read_content(int fildes, char *buf, size_t n);
char *read_headers(int fildes);
long parse_content_length(char *headers);
void document_close(int to_server_fd[2], char *uri);
void print_message(char *json);
void completion(document *doc, server *ser);
void document_change(document *doc, int *to_server_fildes);
void document_open(document *doc, int *to_serve_fd);
void initialize(server *s, char *uri);
cJSON *make_initialize_request(server *s, char *uri);
cJSON *make_initialized_notification(void);
void start_server(int to_child[2], int from_child[2]);
char *get_init_message(char *init_msg);
cJSON *make_did_open(document *doc);
cJSON *make_did_change(document *doc);
cJSON *make_did_close(char *uri);
cJSON *make_completion_request(document *doc, server *ser);
cJSON *make_shutdown_request(server *s);
char *get_uri(char *path);
char *read_file(char *path);
void initialize_document(document *d);

int main(void) {
     server ser;
     ser.ID = 1;
     start_server(ser.to_server_fd, ser.to_client_fd);

     document doc;
     initialize_document(&doc);

     initialize(&ser, doc.uri);
     document_open(&doc, ser.to_server_fd);
     document_change(&doc, ser.to_server_fd);
     completion(&doc, &ser);
     document_close(ser.to_server_fd, doc.uri);
     halt(&ser);

     free(doc.uri);
     close(ser.to_server_fd[1]);
     close(ser.to_client_fd[0]);
     wait(NULL);
     return 0;
}

void initialize_document(document *doc) {
     doc->file_name = "test.c";
     init_file(doc->file_name);

     char *path = realpath(doc->file_name, NULL);
     assert(path);

     doc->uri = get_uri(path);
     assert(doc->uri);

     doc->language = "c";
     doc->version = 1;

     free(path);
}

void halt(server *s) {
     cJSON *req_shutdown = make_shutdown_request(s);
     send_message(s->to_server_fd[1], req_shutdown);
     cJSON_Delete(req_shutdown);

     char *response = wait_for_response(s->to_client_fd[0]);
     assert(response);
     print_message(response);

     cJSON *notif_exit = cJSON_CreateObject();
     cJSON_AddStringToObject(notif_exit, "jsonrpc", "2.0");
     cJSON_AddStringToObject(notif_exit, "method", "exit");
     send_message(s->to_server_fd[1], notif_exit);
     cJSON_Delete(notif_exit);
}

void print_message(char *json)
{
     cJSON *root = cJSON_Parse(json);
     assert(root);
     char *pretty = cJSON_Print(root); // indented output
     assert(pretty);
     printf("%s\n", pretty);
     free(pretty);
     cJSON_Delete(root);
}

void init_file(char *name) {
     FILE *fp = fopen(name, "w");
     assert(fp);
     fputs("#include <stdio.h>\n\nint main(void) {\n\tprintf(\"hello\");\n\t", fp);
     fclose(fp);
}

void document_close(int to_server_fd[2], char *uri) {
     cJSON *notif_close = make_did_close(uri);
     send_message(to_server_fd[1], notif_close);
     cJSON_Delete(notif_close);
}

void completion(document *doc, server *ser) {
     cJSON *req_completion = make_completion_request(doc, ser);
     send_message(ser->to_server_fd[1], req_completion);
     cJSON_Delete(req_completion);

     char *response = wait_for_response(ser->to_client_fd[0]);
     assert(response);
     print_message(response);
}


void document_change(document *doc, int *to_server_fildes) {
     // some change happens here
     update_document_text();
     cJSON *notif_change = make_did_change(doc);
     send_message(to_server_fildes[1], notif_change);
     cJSON_Delete(notif_change);

}


void document_open(document *doc, int *to_serve_fd) {
     char *initial_text = read_file(doc->file_name);
     cJSON *notif_open = make_did_open(doc);
     send_message(to_serve_fd[1], notif_open);
     cJSON_Delete(notif_open);
     free(initial_text);
}

void start_server(int *to_server_fd, int *to_client_fd) {
     pipe(to_server_fd);
     pipe(to_client_fd);

     // child process (server)
     if (!fork()) {
	  // set read and write to stdin and stdout
	  dup2(to_server_fd[0], 0);
	  dup2(to_client_fd[1], 1);

	  close(to_server_fd[0]);
	  close(to_server_fd[1]);
	  close(to_client_fd[0]);
	  close(to_client_fd[1]);

	  execlp("clangd", "clangd", NULL);
     }

     // parent closes other pipe ends
     close(to_server_fd[0]);
     close(to_client_fd[1]);
}

void send_message(int fildes, cJSON *obj) {
     char *body = cJSON_PrintUnformatted(obj);
     assert(body);
     size_t length = strlen(body);

     char header[128];
     int header_len = snprintf(header, sizeof(header), "Content-Length: %zu\r\n\r\n", length);

     // write header then body
     write(fildes, header, header_len);
     write(fildes, body, length);
     free(body);
}

void initialize(server *s, char *uri) {
     cJSON *req_initialize = make_initialize_request(s, uri);
     send_message(s->to_server_fd[1], req_initialize);
     cJSON_Delete(req_initialize);

     char *response = wait_for_response(s->to_client_fd[0]);
     assert(response);
     print_message(response);
     free(response);

     cJSON *notif_initialized = make_initialized_notification();
     send_message(s->to_server_fd[1], notif_initialized);
     cJSON_Delete(notif_initialized);

}

char *wait_for_response(int fildes) {
     char *headers = read_headers(fildes);
     assert(headers);

     long body_len = parse_content_length(headers);
     assert(body_len);

     char *buffer = malloc(body_len + 1);
     assert(buffer);

     size_t n = read_content(fildes, buffer, body_len);
     assert(n == body_len);
     buffer[body_len] = '\0';

     free(headers);
     return buffer;
}

ssize_t read_content(int fildes, char *buf, size_t n) {
     ssize_t bytes = 0;
     while (bytes < n) {
	  ssize_t r = read(fildes, buf + bytes, n - bytes);
	  // EOF
	  if (r == 0) {
	       break;
	  }
	  else {
	       bytes += r;
	  }
     }
     return bytes;
}

long parse_content_length(char *headers) {
    char *end = strstr(headers, "\r\n\r\n");
    if (!end) {
	 end = headers + strlen(headers);
    }

    char *p = headers;
    while (p < end) {
        char *line_end = strstr(p, "\r\n");
        if (!line_end) {
	     line_end = end;
	}

        if (strncasecmp(p, "Content-Length:", 15) == 0) {
            char *ep = NULL;
	    long n = -1;
            n = strtol(p + 15, &ep, 10);
	    if (n >= 0) {
		 return n;
	    }
	    else {
		 return -1;
	    }
        }

        if (line_end == end) {
	     break;
	}
        p = line_end + 2;
    }
    return -1;
}

char *read_headers(int fildes) {
     size_t max = 512;
     size_t len = 0;
     char *buf = malloc(max);
     assert(buf);

     while (1) {
	  char c;
	  ssize_t r = read(fildes, &c, 1);
	  assert(r);
	  // EOF
	  if (r == 0) {
	       free(buf);
	       return NULL;
	  }
	  // realloc the buffer
	  if (len + 1 >= max) {
	       size_t ncap = max * 2;
	       char *nb = realloc(buf, ncap);
	       assert(nb);
	       buf = nb;
	       max = ncap;
	  }
	  buf[len++] = c;

	  // end of header
	  if (len > 3 && buf[len - 4] == '\r' && buf[len - 3] == '\n' &&
			 buf[len - 2] == '\r' && buf[len - 1] == '\n') {
	       break;
	  }
     }
     buf[len] = '\0';

     return buf;
}

char *get_uri(char *path) {
     char *uri_root = "file:///%s";
     size_t length = strlen(path);
     char *uri = malloc(length + 9);

     sprintf(uri, uri_root, path);
     return uri;
}

char *read_file(char *path) {
     FILE *fp = fopen(path, "r");
     assert(fp);
     fseek(fp, 0, SEEK_END);
     long n = ftell(fp);
     rewind(fp);
     char *buf = malloc(n + 1);

     fread(buf, 1, (size_t)n, fp);
     buf[n] = '\0';
     fclose(fp);
     return buf;
}

int get_id(server *s) {
     return s->ID++;
}

int get_version(document *d) {
     return d->version++;
}

// TODO
void update_document_text(void) {
     ;
     // placeholder function... go find changes to text document and update the document struct
}


cJSON *make_initialize_request(server *s, char *uri) {
     cJSON *req = cJSON_CreateObject();
     cJSON_AddStringToObject(req, "jsonrpc", "2.0");
     cJSON_AddNumberToObject(req, "id", get_id(s));
     cJSON_AddStringToObject(req, "method", "initialize");

     cJSON *params = cJSON_CreateObject();
     cJSON_AddItemToObject(req, "params", params);

     cJSON_AddNumberToObject(params, "processId", getpid());
     cJSON_AddStringToObject(params, "rootUri", uri);
     cJSON_AddItemToObject(params, "capabilities", cJSON_CreateObject());
     return req;
}

cJSON *make_initialized_notification(void) {
     cJSON *n = cJSON_CreateObject();
     cJSON_AddStringToObject(n, "jsonrpc", "2.0");
     cJSON_AddStringToObject(n, "method", "initialized");
     return n;
}

// NOTE up to here
// TODO add these in as actual parameters
cJSON *make_did_open(document *doc) {
     cJSON *n = cJSON_CreateObject();
     cJSON_AddStringToObject(n, "jsonrpc", "2.0");
     cJSON_AddStringToObject(n, "method", "textDocument/didOpen");

     cJSON *params = cJSON_CreateObject();
     cJSON_AddItemToObject(n, "params", params);

     cJSON *text_document = cJSON_CreateObject();
     cJSON_AddItemToObject(params, "textDocument", text_document);
     cJSON_AddStringToObject(text_document, "uri", doc->uri);
     cJSON_AddStringToObject(text_document, "languageId", doc->language);
     cJSON_AddNumberToObject(text_document, "version", doc->version);
     cJSON_AddStringToObject(text_document, "text", doc->text);
     return n;
}

cJSON *make_did_change(document *doc) {
     update_document_text();

     cJSON *n = cJSON_CreateObject();
     cJSON_AddStringToObject(n, "jsonrpc", "2.0");
     cJSON_AddStringToObject(n, "method", "textDocument/didChange");

     cJSON *params = cJSON_CreateObject();
     cJSON_AddItemToObject(n, "params", params);

     cJSON *text_document = cJSON_CreateObject();
     cJSON_AddItemToObject(params, "textDocument", text_document);
     cJSON_AddStringToObject(text_document, "uri", doc->uri);
     cJSON_AddNumberToObject(text_document, "version", doc->version);

     cJSON *changes = cJSON_CreateArray();
     cJSON_AddItemToObject(params, "contentChanges", changes);

     cJSON *change = cJSON_CreateObject();
     cJSON_AddItemToArray(changes, change);
     cJSON_AddStringToObject(change, "text", doc->text);
     return n;
}

cJSON *make_did_close(char *uri) {
     cJSON *close = cJSON_CreateObject();
     cJSON_AddStringToObject(close, "jsonrpc", "2.0");
     cJSON_AddStringToObject(close, "method", "textDocument/didClose");

     cJSON *params = cJSON_CreateObject();
     cJSON_AddItemToObject(close, "params", params);

     cJSON *text_document = cJSON_CreateObject();
     cJSON_AddItemToObject(params, "textDocument", text_document);
     cJSON_AddStringToObject(text_document, "uri", uri);
     return close;
}

cJSON *make_completion_request(document *doc, server *ser) {
     cJSON *req = cJSON_CreateObject();
     cJSON_AddStringToObject(req, "jsonrpc", "2.0");
     cJSON_AddNumberToObject(req, "id", get_id(ser));
     cJSON_AddStringToObject(req, "method", "textDocument/completion");

     cJSON *params = cJSON_CreateObject();
     cJSON_AddItemToObject(req, "params", params);

     cJSON *text_document = cJSON_CreateObject();
     cJSON_AddItemToObject(params, "textDocument", text_document);
     cJSON_AddStringToObject(text_document, "uri", doc->uri);

     cJSON *pos = cJSON_CreateObject();
     cJSON_AddItemToObject(params, "position", pos);
     cJSON_AddNumberToObject(pos, "line", doc->line);
     cJSON_AddNumberToObject(pos, "character", doc->column);

     return req;
}

cJSON *make_shutdown_request(server *ser) {
     cJSON *req = cJSON_CreateObject();
     cJSON_AddStringToObject(req, "jsonrpc", "2.0");
     cJSON_AddNumberToObject(req, "id", get_id(ser));
     cJSON_AddStringToObject(req, "method", "shutdown");
     return req;
}
