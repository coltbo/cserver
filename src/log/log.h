#ifndef LOG_H
#define LOG_H

enum LogLevel {
  Debug,
  Information,
  Warning,
  Error
};

struct LoggerConfig {
  enum LogLevel level;
};

void logger_init(struct LoggerConfig config);
void logger_log(enum LogLevel level, char *fmt, ...);

#endif  // LOG_H

