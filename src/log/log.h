#ifndef LOG_H
#define LOG_H

#define DBG "Debug"
#define INF "Information"
#define WRN "Warning"
#define ERR "Error"

typedef enum {
  Debug,
  Information,
  Warning,
  Error
} LogLevel;

struct LoggerConfig {
  LogLevel level;
};

void logger_init(struct LoggerConfig config);
void logger_log(LogLevel level, char *fmt, ...);

#endif  // LOG_H

