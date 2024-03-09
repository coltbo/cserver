#include "config.h"
#include "../util.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <toml.h>

#define MAX_MIMES 4000

// defaults section
const static int port = 80;
const static char *webroot = "/usr/share/cserver/html";
const static LogLevel loglevel = Warning;
const static MimeType mimes[] = {
    {.file = "txt", .mime = "text/plain"},
    {.file = "html", .mime = "text/html"},
    {.file = "htm", .mime = "text/html"},
    {.file = "csv", .mime = "text/csv"},
    {.file = "css", .mime = "text/css"},
    {.file = "gz", .mime = "application/gzip"},
    {.file = "gif", .mime = "image/gif"},
    {.file = "jpeg", .mime = "image/jpeg"},
    {.file = "jpg", .mime = "image/jpeg"},
    {.file = "js", .mime = "text/javascript"},
    {.file = "json", .mime = "application/json"},
    {.file = "mp3", .mime = "audio/mpeg"},
    {.file = "mp4", .mime = "video/mp4"},
    {.file = "mpeg", .mime = "video/mpeg"},
    {.file = "odp", .mime = "application/vnd.oasis.opendocument.presentation"},
    {.file = "ods", .mime = "application/vnd.oasis.opendocument.spreadsheet"},
    {.file = "odt", .mime = "application/vnd.oasis.opendocument.text"},
    {.file = "png", .mime = "image/png"},
    {.file = "pdf", .mime = "application/pdf"},
    {.file = "svg", .mime = "image/svg+xml"},
    {.file = "tar", .mime = "application/x-tar"},
    {.file = "txt", .mime = "text/plain"},
    {.file = "zip", .mime = "applicationn/zip"},
};

struct Config {
  int port;
  char *webroot;
  LogLevel loglevel;
  MimeType mimes[MAX_MIMES];
};

Config *config_alloc(char *path) {
  Config *config = (Config *)calloc(sizeof(Config), 1);
  if (config == NULL) {
    fprintf(stderr, "Failed to allocate space for config... bailing out\n");
    return NULL;
  }

  FILE *file = fopen(path, "r");
  if (file == NULL) {
    fprintf(stderr, "Could not open config file... using defaults\n");
    config->port = port;
    config->loglevel = loglevel;

    config->webroot = malloc(sizeof(char) * strlen(webroot));
    strncpy(config->webroot, config->webroot, strlen(webroot));

    memcpy(config->mimes, mimes, sizeof(mimes)); // This might dump
    return config;
  }

  char errbuf[200] = {0};
  toml_table_t *toml_base = toml_parse_file(file, errbuf, 200);
  if (toml_base != NULL) {
    toml_table_t *server, *http, *logger;

    server = toml_table_in(toml_base, "server");
    toml_datum_t port_opt = toml_int_in(server, "port");
    if (port_opt.ok) {
      config->port = port_opt.u.i;
    } else {
      config->port = port;
    }

    toml_datum_t webroot_opt = toml_string_in(server, "webroot");
    if (webroot_opt.ok) {
      size_t len = (strlen(webroot_opt.u.s) + 1);
      config->webroot = malloc(sizeof(char) * len);
      strncpy(config->webroot, webroot_opt.u.s, strlen(webroot_opt.u.s));
      config->webroot[len - 1] = '\0';
      free(webroot_opt.u.s);
    } else {
      config->webroot = malloc(sizeof(char) * strlen(webroot));
      strncpy(config->webroot, webroot, strlen(webroot));
    }

    http = toml_table_in(toml_base, "http");
    toml_table_t *mimes_opt = toml_table_in(http, "mime");
    if (mimes_opt != NULL) {
      for (int i = 0;; i++) {
        const char *key = toml_key_in(mimes_opt, i);
        if (!key)
          break;

        toml_datum_t value_opt = toml_string_in(mimes_opt, key);
        if (value_opt.ok) {
          const MimeType mime = {.file = key, .mime = value_opt.u.s};
          config->mimes[i] = mime;
        } else {
          fprintf(stderr, "No mime type found for `%s`\n",
                  key); // TODO - fix this dummy
        }
      }
    } else {
      size_t mime_size = sizeof(mimes) / sizeof(MimeType);
      for (int i = 0; i < mime_size; i++) {
        config->mimes[i] = mimes[i];
      }
    }

    logger = toml_table_in(toml_base, "logger");
    toml_datum_t loglevel_opt = toml_string_in(logger, "loglevel");
    if (loglevel_opt.ok) {
      if (strncmp(loglevel_opt.u.s, DBG, strlen(DBG)) == 0)
        config->loglevel = Debug;
      else if (strncmp(loglevel_opt.u.s, INF, strlen(INF)) == 0)
        config->loglevel = Information;
      else if (strncmp(loglevel_opt.u.s, WRN, strlen(WRN)) == 0)
        config->loglevel = Warning;
      else if (strncmp(loglevel_opt.u.s, ERR, strlen(ERR)) == 0)
        config->loglevel = Error;
      else
        config->loglevel = Warning;
    } else {
      config->loglevel = loglevel;
    }

    free(toml_base);
  } else {
    fprintf(stderr, "%s\n", errbuf);
  }

  fclose(file);

  return config;
}

char *config_get_file_mime_type(struct Config *config, char *path) {
  char *ext = get_file_extension(path);

  size_t mimes_size = sizeof(mimes) / sizeof(MimeType);
  for (int i = 0; i < mimes_size; i++) {
    const char *file = config->mimes[i].file;
    if (strncmp(file, ext, strlen(file)) == 0) {
      return config->mimes[i].mime;
    }
  }

  return NULL;
}

LogLevel config_get_log_level(struct Config *config) {
  return config->loglevel;
}

int config_get_port(struct Config *config) { return config->port; }

char *config_get_webroot(struct Config *config) { return config->webroot; }

void config_free(struct Config *config) { free(config); }
