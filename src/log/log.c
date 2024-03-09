#include "log.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#define MAX_TIME_STAMP 30
#define MAX_LOG_SIZE 8192

static struct LoggerConfig logger = {.level = Warning};

void logger_init(struct LoggerConfig config) { logger = config; }

bool should_log(LogLevel level) {
  return level >= logger.level ? true : false;
}

/* produces a timestamp in the format [dd mm yyyy hh:mm:ss] */
void get_current_timestamp(char *timestamp, size_t len) {
  time_t rawtime;
  struct tm *timeinfo;

  time(&rawtime);
  timeinfo = localtime(&rawtime);

  snprintf(timestamp, len, "[%d/%d/%d %d:%d:%d]", timeinfo->tm_mday,
           timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour,
           timeinfo->tm_min, timeinfo->tm_sec);
}

void logmsg(LogLevel level, char *fmt, va_list args) {
  char buffer[MAX_LOG_SIZE] = {0};
  int bytes = vsnprintf(buffer, MAX_LOG_SIZE, fmt, args);
  if (bytes <= 0) {
    printf("[error] failed to create log message\n");
    return;
  }

  char timestamp[MAX_TIME_STAMP] = {0};
  get_current_timestamp(timestamp, MAX_TIME_STAMP);

  switch (level) {
  case Debug:
    printf("[DBG] %s %s", timestamp, buffer);
    break;
  case Information:
    printf("[INF] %s %s", timestamp, buffer);
    break;
  case Warning:
    printf("[WRN] %s %s", timestamp, buffer);
    break;
  case Error:
    printf("[ERR] %s %s", timestamp, buffer);
    break;
  default:
    printf("[WRN] %s %s", timestamp, buffer);
    break;
  }
}

void logger_log(LogLevel level, char *fmt, ...) {
  if (!should_log(level))
    return;

  va_list args;
  va_start(args, fmt);

  logmsg(level, fmt, args);
}
