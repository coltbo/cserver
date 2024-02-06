#ifndef CONFIG_H
#define CONFIG_H
#include <toml.h>
#include "../log/log.h"

struct Config {
  toml_table_t *server;
  toml_table_t *http;
  toml_table_t *logging;
};

struct Config *config_alloc(char *path);
enum LogLevel config_get_log_level(struct Config *config);
void config_free(struct Config *config);

#endif
