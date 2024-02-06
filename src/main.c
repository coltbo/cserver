#include "server.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: server [PORT]\n");
    exit(EXIT_FAILURE);
  }

  int port = atoi(argv[1]);

  if (!server_run(port)) {
    fprintf(stderr, "[error] an error occurred when running the server\n");
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}
