#include "server.h"
#include "conf/config.h"
#include "http/http.h"
#include "lex/scan.h"
#include "lex/token.h"
#include "log/log.h"
#include <asm-generic/errno.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <toml.h>
#include <unistd.h>

#define MAX_EVENTS 10

static int _severfd;
static struct Config *_config = NULL;

void handler(int signo, siginfo_t *info, void *context) {
  logger_log(Information, "closing sockets...\n");
  close(_severfd);
  logger_log(Information, "sockets closed\n");
}

void server_handle_get(char *req_path, HttpResponse *res) {
  // add file contents to response body
  int rc = http_response_file_to_body(res, req_path);
  if (rc == 0) {
    logger_log(Debug, "read %ld bytes from %s\n", res->body_len, req_path);

    // add Content-Type header to response
    char *mime = config_get_file_mime_type(_config, req_path);
    if (mime != NULL) {
      if (http_response_add_header(res, "Content-Type", mime) != 0) {
        logger_log(Error, "failed to add 'Content-Type' header\n");
      }
      free(mime);
    } else {
      logger_log(
          Warning,
          "failed to find mime type in configuration... using defautl\n");
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

void server_handle_head(char *req_path, HttpResponse *res) {
  if (access(req_path, R_OK) != 0) {
    logger_log(Error, "could not open requested file\n");
    res->status = SERVER_ERROR;
    res->status_msg = "Internal Server Error";
    return;
  }

  FILE *stream;
  if ((stream = fopen(req_path, "rb")) != NULL) {
    // get file size to determine response buff size
    fseek(stream, 0, SEEK_END);
    long cont_len = ftell(stream);
    fseek(stream, 0, SEEK_SET);

    // add Content-Length header
    char len_header[50];
    snprintf(len_header, 50, "%ld",
             cont_len); // TODO - should probably handle this better
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

bool check_for_keep_alive(TokenArray *tarray) {
  Token *connection_token = find_value_token_for_header(tarray, "Connection");
  if (connection_token != NULL) {
    if (strncmp(connection_token->lexeme, "close",
                strlen(connection_token->lexeme)) != 0) {
      return true;
    } else {
      return false;
    }
  } else {
    return true; // we assume true for all good connections
  }
}

void handle_request(int clientfd, char *webroot) {
  char request[MAX_REQUEST] = {0};
  ssize_t valread = read(clientfd, request, MAX_REQUEST);

  if (valread == 0) {
    logger_log(Debug, "client `%d` disconnected... closing connection\n",
               clientfd);
    close(clientfd);
    return;
  } else if (valread == -1) {
    if (errno == EWOULDBLOCK) {
      logger_log(Debug, "no data to read from client `%d`\n", clientfd);
    } else {
      logger_log(
          Error,
          "failed to read message from client `%d`... closing connection\n",
          clientfd);
      close(clientfd);
    }

    return;
  }

  logger_log(Information, "received http request:\n%s\n", request);

  TokenArray *tarray = scan(request);
  HttpResponse *res = http_response_alloc();

  char *resbuf;
  Token *method_token = find_token_by_type(tarray, METHOD);
  Token *uri_token = find_token_by_type(tarray, URI);
  Token *version_token = find_token_by_type(tarray, VERSION);

  Method method;
  bool keep_alive = false;

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
    http_response_add_header(res, "Connection", "close");
    size_t len = http_response_to_str(res, &resbuf);
    send(clientfd, resbuf, strlen(resbuf), 0);
  } else if (!is_http_version_compat(version_token->lexeme)) {
    logger_log(Error, "unsupported http version\n");
    res->status = NOT_SUPPORTED;
    res->status_msg = "HTTP Version Not Supported";
    http_response_add_header(res, "Connection", "close");
    size_t len = http_response_to_str(res, &resbuf);
    send(clientfd, resbuf, strlen(resbuf), 0);
  } else if ((method = method_supported(method_token->lexeme)) == UNSUPPORTED) {
    logger_log(Error, "unsuported http method... rejecting\n");
    res->status = NOT_ALLOWED;
    res->status_msg = "Method Not Allowed";
    http_response_add_header(res, "Connection", "close");
    size_t len = http_response_to_str(res, &resbuf);
    send(clientfd, resbuf, strlen(resbuf), 0);
  } else {
    logger_log(Information, "handling %s request for uri %s\n",
               method_token->lexeme, uri_token->lexeme);

    // check for close connection
    if ((keep_alive = check_for_keep_alive(tarray))) {
      http_response_add_header(res, "Connection", "keep-alive");
    } else {
      http_response_add_header(res, "Connection", "close");
    }

    // construct uri
    char *uri = (strcmp(uri_token->lexeme, "/") == 0) ? "/index.html"
                                                      : uri_token->lexeme;

    int uri_len = strlen(webroot) + strlen(uri) + 1;
    char req_path[uri_len];
    snprintf(req_path, uri_len, "%s%s", webroot, uri);

    switch (method) {
    case GET:
      server_handle_get(req_path, res);
      break;
    case HEAD:
      server_handle_head(req_path, res);
      break;
    default:
      logger_log(Error, "unsupported http method... rejecting\n");
      break;
    }

    size_t len = http_response_to_str(res, &resbuf);
    send(clientfd, resbuf, len, 0);
  }

  if (resbuf != NULL) {
    free(resbuf);
    resbuf = NULL;
  }
  http_response_free(res);
  free_token_array(tarray);

  if (keep_alive == false) {
    logger_log(Information, "closing connection to client `%d`\n", clientfd);
    close(clientfd);
  }
}

bool set_socket_non_blocking(int fd) {
  if (fd < 0)
    return false;

  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1)
    return false;
  flags = flags &
          ~O_NONBLOCK; // bitwise operation? should probably research this more
  return (fcntl(fd, F_SETFL, flags) == 0);
}

int server_run(struct Config *config, struct LoggerConfig *logger) {
  // set local static config
  _config = config;

  int port = config_get_port(_config);
  logger_log(Information, "listening for requests on port `%d`\n", port);

  char *webroot = config_get_webroot(_config);
  logger_log(Information, "serving static files from `%s`\n", webroot);

#define exit_success()                                                         \
  {                                                                            \
    free(webroot);                                                             \
    return 0;                                                                  \
  }

#define exit_error(code, kind)                                                 \
  {                                                                            \
    free(webroot);                                                             \
    perror(kind);                                                              \
    return code;                                                               \
  }

  struct sockaddr_in address;
  socklen_t addrlen = sizeof(address);
  struct sigaction act = {0};

  act.sa_flags = SA_SIGINFO;
  act.sa_sigaction = &handler;
  if (sigaction(SIGINT, &act, NULL) == -1) {
    exit_error(1, "sigaction");
  }

  if ((_severfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    exit_error(2, "socket failed");
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  /* bind the address and port number to a socket */
  if (bind(_severfd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    exit_error(2, "bind failed");
  }

  // setup epoll
  struct epoll_event ev, events[MAX_EVENTS];
  int epollfd = epoll_create1(EPOLL_CLOEXEC);
  if (epollfd == -1) {
    exit_error(3, "epoll_create1");
  }

  /* waiting for client to initiate a connection. The second parameter
     specifies the allowed backlog of requests on the socket.*/
  if (listen(_severfd, SOMAXCONN) < 0) {
    exit_error(2, "listen");
  }

  ev.events = EPOLLIN;
  ev.data.fd = _severfd;
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, _severfd, &ev) == -1) {
    exit_error(3, "epoll_ctl: listen_sock");
  }

  logger_log(Debug, "initialized epoll instance on %d\n", epollfd);
  logger_log(Information, "listening for requests on %d!\n", port);

  for (;;) {
    int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
      exit_error(3, "epoll_wait");
    }

    for (int n = 0; n < nfds; ++n) {
      if (events[n].data.fd == _severfd) {
        logger_log(Debug, "accepting new incoming connections\n");
        int clientfd;
        /* takes the first connection on the socket queue and creates a new
           connected socket with the client.*/
        if ((clientfd =
                 accept(_severfd, (struct sockaddr *)&address, &addrlen)) < 0) {
          exit_error(2, "accept");
        }

        set_socket_non_blocking(clientfd);

        ev.events = EPOLLIN;
        ev.data.fd = clientfd;
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &ev) == -1) {
          exit_error(3, "epoll_ctl: conn_sock");
        }
      } else {
        int clientfd = events[n].data.fd;
        logger_log(Debug, "receiving data from client fd: %d\n", clientfd);

        handle_request(clientfd, webroot);
      }
    }
  }

  exit_success();
}
