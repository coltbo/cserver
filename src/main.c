#include "conf/config.h"
#include "log/log.h"
#include "server.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  // init config
  char *path = "/home/colten/projects/cserver/config.toml";
  struct Config *config = config_alloc(path);
  if (config == NULL) {
    printf("config file not found... using defaults\n");
  }

  // init logger
  struct LoggerConfig logger = {.level = config_get_log_level(config)};
  logger_init(logger);

#define exit_success()                                                         \
  {                                                                            \
    logger_log(Information, "cleaning up resources...\n");                     \
    config_free(config);                                                       \
    logger_log(Information, "finished successfully... Goodbye!\n");            \
    exit(EXIT_SUCCESS);                                                        \
  }

#define exit_fail(code)                                                        \
  {                                                                            \
    logger_log(Information, "cleaning up resources...\n");                     \
    config_free(config);                                                       \
    logger_log(Error, "exiting server with error code `%d`\n", rc);            \
    exit(EXIT_FAILURE);                                                        \
  }

  int rc = 0;
  if ((rc = server_run(config, &logger)) != 0) {
    exit_fail(rc);
  }

  exit_success();
}
