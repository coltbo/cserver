#ifndef CONFIG_H
#define CONFIG_H
#include <toml.h>
#include "../log/log.h"

struct Config {
  toml_table_t *base;
  toml_table_t *server;
  toml_table_t *http;
  toml_table_t *logging;
};

struct Config *config_alloc(char *path);
enum LogLevel config_get_log_level(struct Config *config);
int config_get_port(struct Config *config);
char *config_get_webroot(struct Config *config);
char *config_get_file_mime_type(struct Config *config, char *path);
void config_free(struct Config *config);

#endif
