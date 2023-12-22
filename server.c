#include "server.h"
#include "lex/scan.h"
#include "lex/token.h"
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define RED "\x1B[31m"
#define GRN "\x1B[32m"
#define YEL "\x1B[33m"
#define BLU "\x1B[34m"
#define RESET "\x1B[0m"

// http data
#define MAX_REQUEST 8192
#define MAX_RESPONSE 8192
#define MAX_RES_LINE 100
#define MAX_HEADERS 20
#define HTTP_VERSION "HTTP/1.1"

// http verbs
#define GET "GET"
#define HEAD "HEAD"
#define POST "POST"

int serverfd;

const char *clrf = "\r\n";

struct HttpVersion {
  int major;
  int minor;
};

struct HttpResponse {
  struct HttpVersion version;
  enum StatusCode status;
  char *status_msg;
  char *headers[MAX_HEADERS];
  char *body;
};

struct HttpResponse *http_response_alloc() {
  struct HttpResponse *response =
      (struct HttpResponse *)malloc(sizeof(struct HttpResponse) * 1);
  if (response) {
    memset(response->headers, 0, sizeof(response->headers));
  }
  return response;
}

void http_response_add_header(struct HttpResponse *response, char *header) {
  int i = 0;
  while (response->headers[i] != (void *)0) {
    i++;
  };

  response->headers[i] = (char *)malloc(sizeof(char *) * strlen(header));
  strncpy(response->headers[i], header, strlen(header));
}

void http_response_to_str(struct HttpResponse *response, char *res_buf,
                          size_t buf_size) {
  // append response line
  char res_line[MAX_RES_LINE] = {0};
  snprintf(res_line, MAX_RES_LINE, "%s %d %s\r\n", HTTP_VERSION, OK, "OK");
  strncat(res_buf, res_line, strlen(res_line));

  // append headers
  int i = 0;
  while (response->headers[i] != NULL) {
    char *header = response->headers[i];
    strncat(res_buf, header, strlen(header));
    strncat(res_buf, clrf, strlen(clrf));
    i++;
  }

  // append extra CRLF
  strncat(res_buf, clrf, strlen(clrf));

  // append body
  strncat(res_buf, response->body, strlen(response->body));
}

void http_response_free(struct HttpResponse *response) {
  free(response->body);
  int i = 0;
  while (response->headers[i] != NULL) {
    free(response->headers[i]);
    i++;
  }
  free(response);

  response = NULL;
}

bool is_method_supported(char *method) {
  if (strncmp(method, GET, strlen(GET)) != 0 &&
      strncmp(method, HEAD, strlen(HEAD)) != 0) {
    return false;
  }

  return true;
}

void handler(int signo, siginfo_t *info, void *context) {
  printf("[info] closing sockets...\n");
  close(serverfd);
  printf("[info] sockets closed\n");
}

struct HttpVersion parse_http_version(char *str) {
  int i = 0;
  while (str[i++] != '/')
    ;

  struct HttpVersion version = {.major = atoi(strtok(str + i, ".")),
                                .minor = atoi(strtok(NULL, "."))};

  return version;
}

void handle_request(int clientfd);

int server_run(int port) {
  struct sockaddr_in address;
  socklen_t addrlen = sizeof(address);

  struct sigaction act = {0};

  act.sa_flags = SA_SIGINFO;
  act.sa_sigaction = &handler;
  if (sigaction(SIGINT, &act, NULL) == -1) {
    perror("sigaction");
    return 1;
  }

  if ((serverfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket failed");
    return 1;
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  /* bind the address and port number to a socket */
  if (bind(serverfd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    return 2;
  }

  printf(BLU "[info]" RESET " listening for requests on %d!\n", port);

  for (;;) {
    /* waiting for client to initiate a connection. The second parameter
       specifies the allowed backlog of requests on the socket.*/
    if (listen(serverfd, 3) < 0) {
      perror("listen");
      return 3;
    }

    int clientfd;
    /* takes the first connection on the socket queue and creates a new
       connected socket with the client.*/
    if ((clientfd = accept(serverfd, (struct sockaddr *)&address, &addrlen)) <
        0) {
      perror("accept");
      return 4;
    }

    handle_request(clientfd);
  }
}

void handle_request(int clientfd) {
  char request[MAX_REQUEST] = {0};
  ssize_t valread = read(clientfd, request, MAX_REQUEST);

  struct TokenArray *tarray = scan(request);

  struct Token *method_token = find_token_by_type(tarray, METHOD);
  if (method_token == NULL) {
    fprintf(stderr, RED "[error]" RESET " no method passed in request\n");
    return;
  }

  struct Token *uri_token = find_token_by_type(tarray, URI);
  if (uri_token == NULL) {
    fprintf(stderr, RED "[error]" RESET " no uri passed in request\n");
    return;
  }

  struct Token *version_token = find_token_by_type(tarray, VERSION);

  if (version_token == NULL) {
    fprintf(stderr, RED "[error]" RESET " no HTTP version passed in request\n");
    return;
  }

  struct HttpVersion version = parse_http_version(version_token->lexeme);

  struct HttpResponse *response;
  if ((response = http_response_alloc()) == NULL) {
    fprintf(stderr,
            RED "[error]" RESET " failed to initialize http response\n");
    return;
  }

  response->version = version;

  if (!is_method_supported(method_token->lexeme)) {
    printf(RED "[error]" RESET " unsuported http method; rejecting request\n");
  }

  printf(BLU "[info]" RESET " handling %s request for uri %s\n",
         method_token->lexeme, uri_token->lexeme);

  http_response_add_header(response, "Content-Type: text/html");

  // TODO - maybe I should have some configuration
  // for the root web directory?
  char *web_root = "/home/colten/projects/cserver/www";
  char *uri =
      (strcmp(uri_token->lexeme, "/") == 0) ? "/index.html" : uri_token->lexeme;
  int uri_len = strlen(web_root) + strlen(uri) + 1;

  char req_path[uri_len];
  snprintf(req_path, uri_len, "%s%s", web_root, uri);

  /* vars used for response */
  char response_line[MAX_RES_LINE] = {0};

  if (access(req_path, R_OK) == 0) {
    FILE *stream;

    if ((stream = fopen(req_path, "r")) == NULL) {
      // TODO - handle server errors
      fprintf(stderr, RED "[error]" RESET " could not open requested file\n");
      response->status = SERVER_ERROR;
      response->status_msg = "Internal Server Error";
    } else {
      // TODO - need to actually generate a http response

      // get file size to determine response buff size
      fseek(stream, 0, SEEK_END);
      long fsize = ftell(stream);
      fseek(stream, 0, SEEK_SET);

      response->body = malloc(sizeof(char) * fsize);
      fread(response->body, sizeof(char), fsize, stream);

      // http_response_add_header(response, "Content-Length: ");

      size_t buf_size = fsize + 1024;

      char *res_buf = malloc(sizeof(char) * (buf_size));

      http_response_to_str(response, res_buf, buf_size);

      send(clientfd, res_buf, buf_size, 0);

      free(res_buf);
      res_buf = NULL;
    }

    fclose(stream);
  } else {
    snprintf(response_line, MAX_RES_LINE, "%s %s\r\n", HTTP_VERSION,
             "Not Found");
    send(clientfd, response_line, strlen(response_line), 0);
  }

  http_response_free(response);
  free_token_array(tarray);

  close(clientfd);
}
