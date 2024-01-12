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

int serverfd;
toml_table_t *config = NULL;

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

  return config;
}

void handle_request(int clientfd);

int server_run(int port) {
  if ((config = server_get_config()) == NULL) {
    fprintf(stderr, "[error] failed to read config file... bailing out\n");
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

    toml_value_t webroot = toml_table_string(config, "webroot");
    if (!webroot.ok) {
      webroot.u.s = "/home";
    }

    char *uri = (strcmp(uri_token->lexeme, "/") == 0) ? "/index.html"
                                                      : uri_token->lexeme;
    int uri_len = strlen(webroot.u.s) + strlen(uri) + 1;

    char req_path[uri_len];
    snprintf(req_path, uri_len, "%s%s", webroot.u.s, uri);
    
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
        res->body[fsize] = '\0';

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
