#include "util.h"
#include <string.h>

char *get_file_extension(char *path) {
  int i = strlen(path) - 1;
  while (path[i] != '.' && path[i] >= 0)
    i--;

  return &path[i + 1];
}
