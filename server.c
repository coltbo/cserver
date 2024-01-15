#include "server.h"
#include "http/http.h"
#include "lex/scan.h"
#include "lex/token.h"
#include "toml-c.h"
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

#define MAX_READ_BUF 100

int serverfd;
toml_table_t *config = NULL;     // base config
toml_table_t *serverconf = NULL; // server specific values

void handler(int signo, siginfo_t *info, void *context) {
  printf("[info] closing sockets...\n");
  close(serverfd);
  printf("[info] sockets closed\n");
}

toml_table_t *server_get_config() {
  FILE *file;
  toml_table_t *config = NULL;
  char errbuf[200] = {0};

  char *path = "/home/colten/Projects/cserver/config.toml";
  if ((file = fopen(path, "r")) == NULL) {
    fprintf(stderr, "[error] failed to open config file\n");
    return config;
  }

  if ((config = toml_parse_file(file, errbuf, 200)) == NULL) {
    fprintf(stderr, "[error] %s\n", errbuf);
  }

  fclose(file);
  return config;
}

char *get_file_extesion(char *path) {
  int i = strlen(path) - 1;
  while (path[i] != '.' && path[i] >= 0)
    i--;

  return &path[i + 1];
}

toml_value_t get_file_mime_type(char *path) {
  toml_table_t *mimes = toml_table_table(serverconf, "mime");

  char *ext = get_file_extesion(path);
  toml_value_t value = toml_table_string(mimes, ext);

  return value;
}

void handle_request(int clientfd);

int server_run(int port) {
  if ((config = server_get_config()) == NULL) {
    fprintf(stderr, "[error] failed to read config file... bailing out\n");
    return 1;
  }

  if ((serverconf = toml_table_table(config, "server")) == NULL) {
    fprintf(stderr,
            "[error] failed to read server config table... bailing out\n");
    return 1;
  }

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

  toml_free(config);
  config = NULL;
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
    size_t len = http_response_to_str(res, &resbuf);
    send(clientfd, resbuf, strlen(resbuf), 0);
  } else if (!is_http_version_compat(version_token->lexeme)) {
    fprintf(stderr, "[error] unsupported http version\n");
    res->status = NOT_SUPPORTED;
    res->status_msg = "HTTP Version Not Supported";
    size_t len = http_response_to_str(res, &resbuf);
    send(clientfd, resbuf, strlen(resbuf), 0);
  } else if (!is_method_supported(method_token->lexeme)) {
    fprintf(stderr, "[error] unsuported http method... rejecting\n");
    res->status = NOT_ALLOWED;
    res->status_msg = "Method Not Allowed";
    size_t len = http_response_to_str(res, &resbuf);
    send(clientfd, resbuf, strlen(resbuf), 0);
  } else {
    printf("[info] handling %s request for uri %s\n", method_token->lexeme,
           uri_token->lexeme);

    toml_value_t webroot = toml_table_string(serverconf, "webroot");
    if (!webroot.ok) {
      webroot.u.s = "/home";
    }

    printf("[info] serving static files from '%s'\n", webroot.u.s);

    char *uri = (strcmp(uri_token->lexeme, "/") == 0) ? "/index.html"
                                                      : uri_token->lexeme;
    int uri_len = strlen(webroot.u.s) + strlen(uri) + 1;

    char req_path[uri_len];
    snprintf(req_path, uri_len, "%s%s", webroot.u.s, uri);

    free(webroot.u.s);
    webroot.u.s = NULL;

    if (access(req_path, R_OK) == 0) {
      FILE *stream;

      if ((stream = fopen(req_path, "rb")) == NULL) {
        fprintf(stderr, "[error] could not open requested file\n");
        res->status = SERVER_ERROR;
        res->status_msg = "Internal Server Error";
        size_t len = http_response_to_str(res, &resbuf);
        send(clientfd, resbuf, strlen(resbuf), 0);
      } else {
        res->status = OK;
        res->status_msg = "OK";

        // add Content-Type header to response
        toml_value_t mime = get_file_mime_type(req_path);
        if (mime.ok) {
          if (http_response_add_header(res, "Content-Type", mime.u.s) != 0) {
            fprintf(stderr, "[error] failed to add 'Content-Type' header\n");
          }
        } else {
          if (http_response_add_header(res, "Content-Type",
                                       "application/octet-stream") != 0) {
            fprintf(stderr, "[error] failed to add 'Content-Type' header\n");
          }
        }

        // get file size to determine response buff size
        fseek(stream, 0, SEEK_END);
        res->body_len = ftell(stream);
        fseek(stream, 0, SEEK_SET);

        res->body = malloc(sizeof(unsigned char) * res->body_len);
        memset(res->body, 0, res->body_len);

        int offset = 0;
        size_t bytes_left = res->body_len;
        unsigned long nread = 0;
        unsigned char buf[MAX_READ_BUF] = {0};
        while ((nread = fread(buf, 1, MAX_READ_BUF, stream))) {
        
          size_t max = MAX_READ_BUF > bytes_left ? bytes_left : MAX_READ_BUF;
          
          printf("[info] copying %zu bytes into buf from %s\n", max, req_path);
          memcpy(res->body+offset, buf, max);

          offset += max;
          bytes_left -= max;
        }

        size_t len = http_response_to_str(res, &resbuf);
        send(clientfd, resbuf, len, 0);
      }

      fclose(stream);
    } else {
      res->status = NOT_FOUND;
      res->status_msg = "Not Found";
      size_t len = http_response_to_str(res, &resbuf);
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
