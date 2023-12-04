#include "server.h"
#include "lex/scan.h"
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_REQUEST 1024

int serverfd, clientfd;

void handler(int signo, siginfo_t *info, void *context) {
  printf("[info] closing sockets...\n");
  close(clientfd);
  close(serverfd);
  printf("[info] sockets closed\n");
}

void handle_request(char *request);

int server_run(int port) {
  struct sockaddr_in address;
  socklen_t addrlen = sizeof(address);

  struct sigaction act = { 0 };

  act.sa_flags = SA_SIGINFO;
  act.sa_sigaction = &handler;
  if (sigaction(SIGINT, &act, NULL) == - 1) {
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

    /* takes the first connection on the socket queue and creates a new
       connected socket with the client.*/
    if ((clientfd = accept(serverfd, (struct sockaddr *)&address, &addrlen)) <
        0) {
      perror("accept");
      return 4;
    }

    char request[MAX_REQUEST] = {0};

    ssize_t valread = read(clientfd, request, MAX_REQUEST);
    handle_request(request);

    char *reply = "Welcome to the server!";
    send(clientfd, reply, strlen(reply), 0);
    printf("reply message sent\n");

    close(clientfd);
  }
}

void handle_request(char *request) {
  struct TokenArray *tarray = scan(request);

  printf("[debug] number of tokens parsed: %zu\n", tarray->index);

  for (int i = 0; i < tarray->index; i++) {
    printf("[debug] parsed token: {%d, %s}\n", tarray->tokens[i]->type,
           tarray->tokens[i]->lexeme);
  }

  free_token_array(tarray);
}
