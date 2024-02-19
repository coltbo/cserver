#include "config.h"
#include "../util.h"
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

    config->base = toml_parse_file(file, errbuf, 200);
    if (config->base != NULL) {
      if ((config->server = toml_table_in(config->base, "server")) == NULL) {
        printf("Could not find server config... using defaults\n");
      }
      if ((config->http = toml_table_in(config->base, "http")) == NULL) {
        printf("Could not find http config... using defaults\n");
      }
      if ((config->logging = toml_table_in(config->base, "logger")) == NULL) {
        printf("Could not find logging config... using defaults\n");
      }
    } else {
      fprintf(stderr, "%s\n", errbuf);
    }

    fclose(file);
  }

  return config;
}

char *config_get_file_mime_type(struct Config *config, char *path) {
  toml_table_t *mimes = toml_table_in(config->http, "mime");

  char *ext = get_file_extension(path);
  toml_datum_t value = toml_string_in(mimes, ext);
  if (!value.ok) {
    return NULL;
  }

  return value.u.s;
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

char *config_get_webroot(struct Config *config) {
  toml_datum_t webroot = toml_string_in(config->server, "webroot");
  if (!webroot.ok) {
    return NULL;
  }

  return webroot.u.s;
}

void config_free(struct Config *config) {
  toml_free(config->base);
  config->base = NULL;
  config->server = NULL;
  config->http = NULL;
  config->logging = NULL;
  free(config);
}
