#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"

static struct LoggerConfig logger = {.level = Warning};

void logger_init(struct LoggerConfig config) {
  logger = config;
}

bool should_log(enum LogLevel level) {
  return level >= logger.level ? true : false;
}

char *format_input_str(char *fmt, va_list args) {
  char *buffer = malloc(sizeof(char *) * 250);
  if (buffer != NULL) {
    int rc = vsprintf(buffer, fmt, args);
  }

  return buffer;
}

void logmsg(enum LogLevel level, char *fmt, va_list args) {
  char *buffer = format_input_str(fmt, args);
  if (buffer == NULL)
    return;

  switch (level) {
  case Debug:
    printf("[debg] %s", buffer);
    break;
  case Information:
    printf("[info] %s", buffer);
    break;
  case Warning:
    printf("[warn] %s", buffer);
    break;
  case Error:
    printf("[error] %s", buffer);
    break;
  default:
    printf("[warn] %s", buffer);
    break;
  }

  free(buffer);
}

void logger_log(enum LogLevel level, char *fmt, ...) {
  if (!should_log(level))
    return;

  va_list args;
  va_start(args, fmt);

  logmsg(level, fmt, args);
}
