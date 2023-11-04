#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF_SIZE 500

int foot(int x) {
  if (x == 1) return 5;

  return x;
}

int main(int argc, char *argv[]) {
  int sfd, s;
  char buf[BUF_SIZE];
  ssize_t nread;
  socklen_t peer_addrlen;
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  struct sockaddr_storage peer_addr;

  if (argc != 3) {
    fprintf(stderr, "Expected 2 arguments\n");
    exit(EXIT_FAILURE);
  }

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM; /* Stream socket */
  hints.ai_flags = AI_PASSIVE;     /* For wildcard IP address */
  hints.ai_protocol = 0;           /* Any protocol */
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  s = getaddrinfo(argv[1], argv[2], &hints, &result);
  if (s != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    exit(EXIT_SUCCESS);
  }

  /* getaddrinfo() returns a list of address structures.
   * Try each address until we successfully bind(2).
   * If socket(2) (or bind(2)) fails, we (close the socket and) try the next
   * address*/
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    if ((sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol) == -1)) {
	continue;
    }

    if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0) {
      break; /* Success */
    }
    
    close(sfd);
  }

  freeaddrinfo(result);

  if (rp == NULL) {
    fprintf(stderr, "[error] could not bind\n");
    exit(EXIT_FAILURE);
  }

  printf("[info] waiting for connections...\n");

  if (listen(sfd, 3) < 0) {
    fprintf(stderr, "[error] failed on listen to socket\n");
    exit(EXIT_FAILURE);
  }

  int new_socket;
  if ((new_socket = accept(sfd, (struct sockaddr *)&peer_addr, &peer_addrlen)) < 0)
  {
    fprintf(stderr, "[error] failed to accept\n");
    exit(EXIT_FAILURE);
  }

  for (;;) {
    char host[NI_MAXHOST], service[NI_MAXSERV];

    // Reset buffer
    memset(buf, 0, BUF_SIZE);

    peer_addrlen = sizeof(peer_addr);

    nread = recv(
    nread = recvfrom(sfd, buf, BUF_SIZE, 0, (struct sockaddr *)&peer_addr,
                     &peer_addrlen);

    if (nread == -1)
      continue;

    s = getnameinfo((struct sockaddr *)&peer_addr, peer_addrlen, host,
                    NI_MAXHOST, service, NI_MAXSERV, NI_NUMERICSERV);

    if (s == 0) {
      printf("Received %zd bytes from %s:%s\n", nread, host, service);
      printf("Message: %s\n", buf);
    } else {
      fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));
    }

    if (sendto(sfd, buf, nread, 0, (struct sockaddr *)&peer_addr,
               peer_addrlen) != nread) {
      fprintf(stderr, "Error sending response\n");
    }
  }

  return 0;
}
