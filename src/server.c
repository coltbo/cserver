#include "server.h"
#include "conf/config.h"
#include "http/http.h"
#include "lex/scan.h"
#include "lex/token.h"
#include "log/log.h"
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <toml.h>
#include <unistd.h>

int serverfd;
toml_datum_t webroot;
struct Config *config = NULL;

void handler(int signo, siginfo_t *info, void *context) {
  logger_log(Information, "closing sockets...\n");
  close(serverfd);
  logger_log(Information, "sockets closed\n");
}

char *get_file_extesion(char *path) {
  int i = strlen(path) - 1;
  while (path[i] != '.' && path[i] >= 0)
    i--;

  return &path[i + 1];
}

toml_datum_t get_file_mime_type(char *path) {
  toml_table_t *mimes = toml_table_in(config->http, "mime");

  char *ext = get_file_extesion(path);
  toml_datum_t value = toml_string_in(mimes, ext);

  return value;
}

void server_handle_get(char *uri, struct HttpResponse *res) {
  int uri_len = strlen(webroot.u.s) + strlen(uri) + 1;

  char req_path[uri_len];
  snprintf(req_path, uri_len, "%s%s", webroot.u.s, uri);

  // add file contents to response body
  int rc = http_response_file_to_body(res, req_path);
  if (rc == 0) {
    logger_log(Debug, "read %ld bytes from %s\n", res->body_len, req_path);
    
    // add Content-Type header to response
    toml_datum_t mime = get_file_mime_type(req_path);
    if (mime.ok) {
      if (http_response_add_header(res, "Content-Type", mime.u.s) != 0) {
        logger_log(Error, "failed to add 'Content-Type' header\n");
      }
    } else {
      if (http_response_add_header(res, "Content-Type",
                                   "application/octet-stream") != 0) {
        logger_log(Error, "failed to add 'Content-Type' header\n");
      }
    }

    // add Content-Length header
    char len_header[50];
    snprintf(len_header, 50, "%ld",
             res->body_len); // TODO - should probably handle this better
    if (http_response_add_header(res, "Content-Length", len_header) != 0) {
      logger_log(Error, "failed to add 'Content-Length' header\n");
    }
    res->status = OK;
    res->status_msg = "OK";
  } else if (rc == 1) {
    logger_log(Error, "could not open requested file\n");
    res->status = SERVER_ERROR;
    res->status_msg = "Internal Server Error";
  } else if (rc == 2) {
    logger_log(Error, "file not found %s\n", req_path);
    res->status = NOT_FOUND;
    res->status_msg = "Not Found";
  }
}

void server_handle_head(char *uri, struct HttpResponse *res) {
  int uri_len = strlen(webroot.u.s) + strlen(uri) + 1;

  char req_path[uri_len];
  snprintf(req_path, uri_len, "%s%s", webroot.u.s, uri);

  if (access(req_path, R_OK) != 0) {
    logger_log(Error, "could not open requested file\n");
    res->status = SERVER_ERROR;
    res->status_msg = "Internal Server Error";
    return;
  }

  FILE *stream;
  if ((stream = fopen(
		      req_path, "rb")) != NULL) {
    // get file size to determine response buff size
    fseek(stream, 0, SEEK_END);
    long cont_len = ftell(stream);
    fseek(stream, 0, SEEK_SET);

    // add Content-Length header
    char len_header[50];
    snprintf(len_header, 50, "%ld", cont_len); // TODO - should probably handle this better
    if (http_response_add_header(res, "Content-Length", len_header) != 0) {
      logger_log(Error, "failed to add 'Content-Length' header\n");
    }
    res->status = OK;
    res->status_msg = "OK";
  } else {
    logger_log(Error, "file not found %s\n", req_path);
    res->status = NOT_FOUND;
    res->status_msg = "Not Found";    
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

    logger_log(Error, "%s", msg);

    res->status = BAD_REQUEST;
    res->status_msg = "Bad Request";
    size_t len = http_response_to_str(res, &resbuf);
    send(clientfd, resbuf, strlen(resbuf), 0);
  } else if (!is_http_version_compat(version_token->lexeme)) {
    logger_log(Error, "unsupported http version\n");
    res->status = NOT_SUPPORTED;
    res->status_msg = "HTTP Version Not Supported";
    size_t len = http_response_to_str(res, &resbuf);
    send(clientfd, resbuf, strlen(resbuf), 0);
  } else if (!is_method_supported(method_token->lexeme)) {
    logger_log(Error, "unsuported http method... rejecting\n");
    res->status = NOT_ALLOWED;
    res->status_msg = "Method Not Allowed";
    size_t len = http_response_to_str(res, &resbuf);
    send(clientfd, resbuf, strlen(resbuf), 0);
  } else {
    logger_log(Information, "handling %s request for uri %s\n",
               method_token->lexeme, uri_token->lexeme);

    char *uri = (strcmp(uri_token->lexeme, "/") == 0) ? "/index.html"
                                                      : uri_token->lexeme;

    server_handle_get(uri, res);

    size_t len = http_response_to_str(res, &resbuf);
    send(clientfd, resbuf, len, 0);
  }

  if (resbuf != NULL) {
    free(resbuf);
    resbuf = NULL;
  }
  http_response_free(res);
  free_token_array(tarray);
  close(clientfd);
}

int server_run(int port) {
  // get configuration
  char *path = "/home/colten/Projects/cserver/config.toml";
  config = config_alloc(path);

  // initialize logger
  struct LoggerConfig logger = {.level = config_get_log_level(config)};
  logger_init(logger);

  // initialize base web dir
  webroot = toml_string_in(
      config->server, "webroot"); // TODO - have some way to check for nulls
  if (!webroot.ok) {
    webroot.u.s = "/home"; // TODO - set some reasonable default
  }

  logger_log(Information, "serving static files from '%s'\n", webroot.u.s);

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

  logger_log(Information, "listening for requests on %d!\n", port);

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

  free(webroot.u.s);
  config_free(config);
}
