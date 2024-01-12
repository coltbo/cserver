#include "http.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char eol = '\0';
const char *clrf = "\r\n";

struct HttpResponse *http_response_alloc() {
  struct HttpResponse *response =
      (struct HttpResponse *)malloc(sizeof(struct HttpResponse) * 1);
  if (response) {
    response->headers = malloc(sizeof(char *) * MAX_HEADERS);
    for (int i = 0; i < MAX_HEADERS; i++) {
      response->headers[i] = NULL;
    }

    response->body = NULL;
  }
  return response;
}

void http_response_add_header(struct HttpResponse *response, char *header) {
  int i = 0;
  while (response->headers[i] != (void *)0) {
    i++;
  };

  response->headers[i] = malloc(sizeof(char *) * strlen(header));
  strncpy(response->headers[i], header, strlen(header));
}

size_t http_response_calc_header_size(struct HttpResponse *res) {
  int i = 0;
  size_t size = 0;
  while (res->headers[i] != NULL) {
    size += strlen(res->headers[i]) + strlen(clrf);
    i++;
  }

  return size;
}

size_t http_response_size(struct HttpResponse *res) {
  size_t bufsize = MAX_RES_LINE + http_response_calc_header_size(res);
  if (res->body != NULL) {
    bufsize += strlen(clrf) + strlen(res->body);
  }
  return bufsize;
}

char *http_response_to_str(struct HttpResponse *res) {
  size_t bufsize = http_response_size(res);

  char *resbuf = malloc(sizeof(char) * bufsize);
  memset(resbuf, 0, bufsize);

  // append response line
  char res_line[MAX_RES_LINE] = {0};
  snprintf(res_line, MAX_RES_LINE, "%s %d %s\r\n", HTTP_VERSION, res->status,
           res->status_msg);
  strncat(resbuf, res_line, strlen(res_line));

  // append headers
  int i = 0;
  while (res->headers[i] != NULL) {
    char *header = res->headers[i];
    strncat(resbuf, header, strlen(header));
    strncat(resbuf, clrf, strlen(clrf));
    i++;
  }

  // append body
  if (res->body != NULL) {
    strncat(resbuf, clrf, strlen(clrf));
    strncat(resbuf, res->body, (strlen(res->body)));
  }

  return resbuf;
}

void http_response_free(struct HttpResponse *response) {
  free(response->body);
  response->body = NULL;

  int i = 0;
  while (response->headers[i] != NULL) {
    free(response->headers[i]);
    response->headers[i] = NULL;
    i++;
  }

  free(response->headers);
  response->headers = NULL;
  free(response);
  response = NULL;
}

bool is_method_supported(char *method) {
  if (strncmp(method, GET, strlen(GET)) != 0 &&
      strncmp(method, HEAD, strlen(HEAD)) != 0) {
    return false;
  }

  return true;
}

bool is_http_version_compat(char *ver) {
  int i = 0;
  while (ver[i++] != '/')
    ;

  int major = atoi(strtok(ver + i, "."));
  int minor = atoi(strtok(NULL, "."));

  if (major != 1 || minor != 1) {
    return false;
  }
  return true;
}
