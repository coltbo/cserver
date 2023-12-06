#include "server.h"
#include "lex/scan.h"
#include "lex/token.h"
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_REQUEST 8192
#define MAX_RES_LINE 100

int serverfd;

void handler(int signo, siginfo_t *info, void *context) {
  printf("[info] closing sockets...\n");
  close(serverfd);
  printf("[info] sockets closed\n");
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
    fprintf(stderr, "[error] no method passed in request\n");
    return;
  }

  struct Token *uri_token = find_token_by_type(tarray, URI);
  if (uri_token == NULL) {
    fprintf(stderr, "[error] no uri passed in request\n");
    return;
  }

  // TODO - maybe I should have some configuration
  // for the root web directory?
  char *web_root = "/home/colten/projects/cserver/www";
  printf("%s\n", uri_token->lexeme);
  char *uri =
      (strcmp(uri_token->lexeme, "/") == 0) ? "/index.html" : uri_token->lexeme;
  int uri_len = strlen(web_root) + strlen(uri) + 1;

  char req_path[uri_len];
  snprintf(req_path, uri_len, "%s%s", web_root, uri);
  if (access(req_path, R_OK) == 0) {
    FILE *stream;

    if ((stream = fopen(req_path, "r")) == NULL) {
      // TODO - handle server errors
      fprintf(stderr, "[error] could not open requested file\n");
    } else {
      // TODO - need to actually generate a http response
      char read_buffer[MAX_RES_LINE] = {0};
      while (fgets(read_buffer, MAX_RES_LINE, stream)) {
        send(clientfd, read_buffer, strlen(read_buffer), 0);
      }
    }

    fclose(stream);
  } else {
    fprintf(stderr, "[error] couldn't get access to `%s`\n", req_path);
  }

  free_token_array(tarray);

  close(clientfd);
}
