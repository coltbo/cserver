#ifndef CONFIG_H
#define CONFIG_H
#include <toml.h>
#include "../log/log.h"

typedef struct {
  const char *file;
  char *mime;
} MimeType;

typedef struct Config Config;

//struct Config {
//  toml_table_t *base;
//  toml_table_t *server;
//  toml_table_t *http;
//  toml_table_t *logging;
//};

/* Override defaults with what is provided in `path`.
 * If the provided path is `NULL`, use all defaults.
 */
Config *config_alloc(char *path);
LogLevel config_get_log_level(Config *config);
int config_get_port(Config *config);
char *config_get_webroot(Config *config);

/* Get the mime type for a specified file.
 * Returns NULL if no matching mime type is found.
 */
char *config_get_file_mime_type(Config *config, char *path);
void config_free(Config *config);

#endif
