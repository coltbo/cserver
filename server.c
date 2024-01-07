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

const char eol = '\0';
const char *clrf = "\r\n";

int serverfd;

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

  response->headers[i] = malloc(sizeof(char *) * strlen(header));
  strncpy(response->headers[i], header, strlen(header));
}

size_t http_response_calc_header_size(struct HttpResponse *res) {
  int i = 0;
  size_t size = 0;
  while (res->headers[i] != NULL) {
    size += strlen(res->headers[i]) + strlen(clrf);
    i++;
  }

  return size;
}

char *http_response_to_str(struct HttpResponse *res) {
  // calculate buf size
  size_t bufsize = MAX_RES_LINE + http_response_calc_header_size(res);
  if (res->body != NULL) {
    bufsize += strlen(clrf) + strlen(res->body);
  }

  char *resbuf = malloc(sizeof(char) * bufsize);
  memset(resbuf, 0, bufsize);

  // append response line
  char res_line[MAX_RES_LINE] = {0};
  snprintf(res_line, MAX_RES_LINE, "%s %d %s\r\n", HTTP_VERSION, res->status,
           res->status_msg);
  strncat(resbuf, res_line, strlen(res_line));

  // append headers
  int i = 0;
  while (res->headers[i] != NULL) {
    char *header = res->headers[i];
    strncat(resbuf, header, strlen(header));
    strncat(resbuf, clrf, strlen(clrf));
    i++;
  }

  // append body
  if (res->body != NULL) {
    strncat(resbuf, clrf, strlen(clrf));
    strncat(resbuf, res->body, (strlen(res->body)));
  }

  return resbuf;
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
  struct HttpResponse *res = http_response_alloc();

  char *resbuf;
  struct Token *method_token = find_token_by_type(tarray, METHOD);
  struct Token *uri_token = find_token_by_type(tarray, URI);
  struct Token *version_token = find_token_by_type(tarray, VERSION);

  if (method_token == NULL || uri_token == NULL || version_token == NULL) {
    char *msg;
    if (method_token == NULL)
      msg = "no method passed to request... rejecting\n";
    if (uri_token == NULL)
      msg = "no uri passed to request... rejecting\n";
    if (version_token == NULL)
      msg = "no version passed to request... rejecting\n";

    fprintf(stderr, "[error] %s", msg);

    res->status = BAD_REQUEST;
    res->status_msg = "Bad Request";
    resbuf = http_response_to_str(res);
    send(clientfd, resbuf, strlen(resbuf), 0);
  } else if (!is_http_version_compat(version_token->lexeme)) {
    fprintf(stderr, "[error] unsupported http version\n");
    res->status = NOT_SUPPORTED;
    res->status_msg = "HTTP Version Not Supported";
    resbuf = http_response_to_str(res);
    send(clientfd, resbuf, strlen(resbuf), 0);
  } else if (!is_method_supported(method_token->lexeme)) {
    fprintf(stderr, "[error] unsuported http method... rejecting\n");
    res->status = NOT_ALLOWED;
    res->status_msg = "Method Not Allowed";
    resbuf = http_response_to_str(res);
    send(clientfd, resbuf, strlen(resbuf), 0);
  } else {
    printf("[info] handling %s request for uri %s\n", method_token->lexeme,
           uri_token->lexeme);

    // TODO - maybe I should have some configuration
    // for the root web directory?
    char *web_root = "/home/colten/Projects/cserver/www";
    char *uri = (strcmp(uri_token->lexeme, "/") == 0) ? "/index.html"
                                                      : uri_token->lexeme;
    int uri_len = strlen(web_root) + strlen(uri) + 1;

    char req_path[uri_len];
    snprintf(req_path, uri_len, "%s%s", web_root, uri);

    if (access(req_path, R_OK) == 0) {
      FILE *stream;

      if ((stream = fopen(req_path, "r")) == NULL) {
        fprintf(stderr, "[error] could not open requested file\n");
        res->status = SERVER_ERROR;
        res->status_msg = "Internal Server Error";
        resbuf = http_response_to_str(res);
        send(clientfd, resbuf, strlen(resbuf), 0);
      } else {
        res->status = OK;
        res->status_msg = "OK";

        // get file size to determine response buff size
        fseek(stream, 0, SEEK_END);
        long fsize = ftell(stream);
        fseek(stream, 0, SEEK_SET);

        size_t bodysize = fsize + 1;

        res->body = malloc(sizeof(char) * bodysize);
        fread(res->body, sizeof(char), fsize, stream);
        res->body[fsize] = eol;

        resbuf = http_response_to_str(res);
        send(clientfd, resbuf, strlen(resbuf), 0);
      }

      fclose(stream);
    } else {
      res->status = NOT_FOUND;
      res->status_msg = "Not Found";
      resbuf = http_response_to_str(res);
      send(clientfd, resbuf, strlen(resbuf), 0);
    }
  }

  if (resbuf != NULL) {
    free(resbuf);
    resbuf = NULL;
  }
  http_response_free(res);
  free_token_array(tarray);
  close(clientfd);
}
