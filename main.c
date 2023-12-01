#include "lex/scan.h"
#include "lex/token.h"
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF_SIZE 1024
#define PORT 8080

int serverfd, clientfd;

void handler(int signo, siginfo_t *info, void *context) {
  printf("[info] closing sockets...\n");
  close(clientfd);
  close(serverfd);
  printf("[info] sockets closed\n");
}

int main(int argc, char *argv[]) {
  // socket file descriptors
  int server = 0;
  struct sockaddr_in address;
  socklen_t addrlen = sizeof(address);
  char buffer[BUF_SIZE] = {0};

  struct sigaction act = { 0 };

  act.sa_flags = SA_SIGINFO;
  act.sa_sigaction = &handler;
  if (sigaction(SIGINT, &act, NULL) == - 1) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }
  
  /* the socket function takes three parameters:
     domain: integer, specifies the communication domain.
             AF_LOCAL is used for comms on the same computer.
             Other types include AF_INET and AF_INET6 for IPv4 and IPv6
     respectively type: communication type. Either SOCK_STREAM or SOCK_UDP.
     Using SOCK_STREAM for TCP connections. protocol: protocol value for IP,
     which is 0.
   */
  if ((serverfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  /* bind the address and port number to a socket */
  if (bind(serverfd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  for (;;) {
    printf("[info] listening on port %d\n", PORT);
    /* waiting for client to initiate a connection. The second parameter
       specifies the allowed backlog of requests on the socket.*/
    if (listen(serverfd, 3) < 0) {
      perror("listen");
      exit(EXIT_FAILURE);
    }

    /* takes the first connection on the socket queue and creates a new
       connected socket with the client.*/
    if ((clientfd = accept(serverfd, (struct sockaddr *)&address, &addrlen)) <
        0) {
      perror("accept");
      exit(EXIT_FAILURE);
    }

    ssize_t valread = read(clientfd, buffer, BUF_SIZE);
    printf("[debug] received: \n%s\n", buffer);
    struct TokenArray *tarray = scan(buffer);

    for (int i = 0; i < tarray->index; i++) {
      printf("[debug] parsed token: {%d, %s}\n", tarray->tokens[i]->type, tarray->tokens[i]->lexeme);
    }

    free_token_array(tarray);

    char *reply = "Welcome to the server!";
    send(clientfd, reply, strlen(reply), 0);
    printf("reply message sent\n");

    close(clientfd);
  }

  close(serverfd);
  return EXIT_SUCCESS;
}
