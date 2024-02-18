#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <toml.h>

struct Config *config_alloc(char *path) {
  struct Config *config = malloc(sizeof(struct Config));
  if (config != NULL) {
    FILE *file;
    char errbuf[200] = {0};
    if ((file = fopen(path, "r")) == NULL) {
      fprintf(stderr, "Could not open config file\n");
    }

    toml_table_t *base = toml_parse_file(file, errbuf, 200);
    if (base != NULL) {
      if ((config->server = toml_table_in(base, "server")) == NULL) {
        printf("Could not find server config... using defaults\n");
      }
      if ((config->http = toml_table_in(base, "http")) == NULL) {
        printf("Could not find http config... using defaults\n");
      }
      if ((config->logging = toml_table_in(base, "logger")) == NULL) {
        printf("Could not find logging config... using defaults\n");
      }
    } else {
      fprintf(stderr, "%s\n", errbuf);
    }

    fclose(file);
  }

  return config;
}

enum LogLevel config_get_log_level(struct Config *config) {
  toml_datum_t loglevel_opt = toml_string_in(config->logging, "loglevel");
  enum LogLevel loglevel;
  if (loglevel_opt.ok) {
    if (strncmp(loglevel_opt.u.s, "Debug", strlen("Debug")) == 0)
      loglevel = Debug;
    else if (strcmp(loglevel_opt.u.s, "Information") == 0)
      loglevel = Information;
    else if (strcmp(loglevel_opt.u.s, "Warning") == 0)
      loglevel = Warning;
    else if (strcmp(loglevel_opt.u.s, "Error") == 0)
      loglevel = Error;
    else
      loglevel = Warning;
  
    free(loglevel_opt.u.s);
  }

  return loglevel;
}

void config_free(struct Config *config) {
  toml_free(config->server);
  toml_free(config->http);
  toml_free(config->logging);
  free(config);
}
