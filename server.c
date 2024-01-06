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
#include <sys/types.h>
#include <unistd.h>

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

bool is_http_version_compat(char *ver) {
  int i = 0;
  while (ver[i++] != '/')
    ;

  int major = atoi(strtok(ver + i, "."));
  int minor = atoi(strtok(NULL, "."));

  if (major != 1 || minor != 1) {
    return false;
  }
  return true;
}

char *http_version_to_str(struct HttpVersion *v) {
  char *vstr = malloc(sizeof(char) * 20);
  snprintf(vstr, 20, "HTTP/%d.%d", v->major, v->minor);
  return vstr;
};

const struct HttpVersion version = {.major = 1, .minor = 1};

struct HttpResponse {
  // struct HttpVersion version;
  enum StatusCode status;
  char *status_msg;
  char **headers;
  char *body;
};

struct HttpResponse *http_response_alloc() {
  struct HttpResponse *response =
      (struct HttpResponse *)malloc(sizeof(struct HttpResponse) * 1);
  if (response) {
    response->headers = malloc(sizeof(char *) * MAX_HEADERS);
    // memset(response->headers, 0, sizeof(MAX_HEADERS));
    for (int i = 0; i < MAX_HEADERS; i++) {
      response->headers[i] = NULL;
    }

    response->body = NULL;
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
  snprintf(res_line, MAX_RES_LINE, "%s %d %s\r\n", HTTP_VERSION,
           response->status, response->status_msg);
  strncat(res_buf, res_line, strlen(res_line));

  // append headers
  int i = 0;
  while (response->headers[i] != NULL) {
    char *header = response->headers[i];
    strncat(res_buf, header, strlen(header));
    strncat(res_buf, clrf, strlen(clrf));
    i++;
  }

  // append body
  if (response->body != NULL) {
    strncat(res_buf, clrf, strlen(clrf));
    strncat(res_buf, response->body, (strlen(response->body) + 1));
  }
}

void http_response_free(struct HttpResponse *response) {
  free(response->body);
  response->body = NULL;

  int i = 0;
  while (response->headers[i] != NULL) {
    free(response->headers[i]);
    response->headers[i] = NULL;
    i++;
  }

  free(response->headers);
  response->headers = NULL;
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

  printf("[info] for requests on %d!\n", port);

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
  struct HttpResponse *response;

#define send_response(res_buf, len)                                            \
  {                                                                            \
    send(clientfd, res_buf, len, 0);                                           \
    http_response_free(response);                                              \
    free_token_array(tarray);                                                  \
    close(clientfd);                                                           \
  }

  if ((response = http_response_alloc()) == NULL) {
    fprintf(stderr, "[error] failed to initialize http response\n");
    char res_line[MAX_RES_LINE];
    snprintf(res_line, MAX_RES_LINE, "%s %d %s\r\n", HTTP_VERSION, SERVER_ERROR,
             "Internal Server Error");
    // send(clientfd, res_line, MAX_RES_LINE, 0);
    send_response(res_line, MAX_RES_LINE);
    return;
  }

  char res_line[MAX_RES_LINE];
  struct Token *method_token;
  struct Token *uri_token;
  struct Token *version_token;

  if ((method_token = find_token_by_type(tarray, METHOD)) == NULL) {
    fprintf(stderr, "[error] no method passed in request\n");
    snprintf(res_line, MAX_RES_LINE, "%s %d %s\r\n", HTTP_VERSION, BAD_REQUEST,
             "Bad Request");
    send(clientfd, res_line, MAX_RES_LINE, 0);
  } else if ((uri_token = find_token_by_type(tarray, URI)) == NULL) {
    fprintf(stderr, "[error] no uri passed in request\n");
    snprintf(res_line, MAX_RES_LINE, "%s %d %s\r\n", HTTP_VERSION, BAD_REQUEST,
             "Bad Request");
    send(clientfd, res_line, MAX_RES_LINE, 0);
  } else if ((version_token = find_token_by_type(tarray, VERSION)) == NULL) {
    fprintf(stderr, "[error] no HTTP version passed in request\n");
    snprintf(res_line, MAX_RES_LINE, "%s %d %s\r\n", HTTP_VERSION, BAD_REQUEST,
             "Bad Request");
    send(clientfd, res_line, MAX_RES_LINE, 0);
  } else if (!is_http_version_compat(version_token->lexeme)) {
    fprintf(stderr, "[error] unsupported http version\n");
    snprintf(res_line, MAX_RES_LINE, "%s %d %s\r\n", HTTP_VERSION, BAD_REQUEST,
             "Bad Request");
    send(clientfd, res_line, MAX_RES_LINE, 0);
  } else {
    char *res_buf;
    
    if (!is_method_supported(method_token->lexeme)) {
      printf("[error] unsuported http method; rejecting request\n");
      res_buf = malloc(sizeof(char) * (MAX_RESPONSE));
      response->status = NOT_ALLOWED;
      response->status_msg = "Method Not Allowed";
      http_response_to_str(response, res_buf, MAX_RESPONSE);
      send(clientfd, res_buf, strlen(res_buf), 0);
    } else {
      printf("[info] handling %s request for uri %s\n", method_token->lexeme,
             uri_token->lexeme);

      http_response_add_header(response, "Content-Type: text/html");

      // TODO - maybe I should have some configuration
      // for the root web directory?
      char *web_root = "/home/colten/projects/cserver/www";
      char *uri = (strcmp(uri_token->lexeme, "/") == 0) ? "/index.html"
                                                        : uri_token->lexeme;
      int uri_len = strlen(web_root) + strlen(uri) + 1;

      char req_path[uri_len];
      snprintf(req_path, uri_len, "%s%s", web_root, uri);

      if (access(req_path, R_OK) == 0) {
        FILE *stream;

        if ((stream = fopen(req_path, "r")) == NULL) {
          fprintf(stderr, "[error] could not open requested file\n");
          response->status = SERVER_ERROR;
          response->status_msg = "Internal Server Error";
          http_response_to_str(response, res_buf, MAX_RESPONSE);
          send(clientfd, res_buf, strlen(res_buf), 0);
        } else {
          // get file size to determine response buff size
          fseek(stream, 0, SEEK_END);
          long fsize = ftell(stream);
          fseek(stream, 0, SEEK_SET);

          response->status = OK;
          response->status_msg = "OK";
          response->body = malloc(sizeof(char) * (fsize + 1));
          fread(response->body, sizeof(char), fsize, stream);
          response->body[fsize] = '\0';

          size_t buf_size =
              fsize + 1 + 1024; // probs not the correct way of doing it
          res_buf = malloc(sizeof(char) * (buf_size));
          memset(res_buf, 0, buf_size);

          http_response_to_str(response, res_buf, buf_size);

          send(clientfd, res_buf, buf_size, 0);
        }

        fclose(stream);
      } else {
        res_buf = malloc(sizeof(char) * (MAX_RESPONSE));
        response->status = NOT_FOUND;
        response->status_msg = "Not Found";
        http_response_to_str(response, res_buf, MAX_RESPONSE);
        send(clientfd, res_buf, strlen(res_buf), 0);
      }
    }
    free(res_buf);
    res_buf = NULL;
  }

  http_response_free(response);
  free_token_array(tarray);
  close(clientfd);
}
