#ifndef SERVER_H
#define SERVER_H
#include "conf/config.h"
#include "log/log.h"

/* Starts the server, listening on the configured port.
   Return codes:
     0 - ok
     1 - sigaction handler
     2 - socket issue
     3 - epoll issue */
int server_run(struct Config *config, struct LoggerConfig *logger);

#endif
