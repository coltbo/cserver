#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF_SIZE 500

int main(int argc, char *argv[]) {
  int sfd, s;
  char buf[BUF_SIZE];
  size_t len;
  ssize_t nread;
  struct addrinfo hints;
  struct addrinfo *result, *rp;

  if (argc != 4) {
    fprintf(stderr, "[error] expected 3 arguments\n");
    exit(EXIT_FAILURE);
  }

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;
  hints.ai_protocol = 0;

  s = getaddrinfo(argv[1], argv[2], &hints, &result);
  if (s != 0) {
    fprintf(stderr, "[error] getaddrinfo: %s\n", gai_strerror(s));
    exit(EXIT_FAILURE);
  }

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sfd == -1)
      continue;

    if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
      break;

    close(sfd);
  }

  freeaddrinfo(result);

  if (rp == NULL) {
    fprintf(stderr, "[error] could not connect\n");
    exit(EXIT_FAILURE);
  }
  
  len = strlen(argv[3]);

  if (len > BUF_SIZE)
  {
    fprintf(stderr, "[error] message too big for buffer\n");
    exit(EXIT_FAILURE);
  }

  if (write(sfd, argv[3], len) != len) {
    fprintf(stderr, "[error] partial/failed write\n");
    exit(EXIT_FAILURE);
  }

  nread = read(sfd, buf, BUF_SIZE);
  if (nread == -1) {
    perror("read");
    exit(EXIT_FAILURE);
  }

  printf("[info] received %zd bytes: %s\n", nread, buf);

  exit(EXIT_SUCCESS);
}
