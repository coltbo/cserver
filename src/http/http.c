#include "http.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_READ_BUF 100

const char eol = '\0';
const char *clrf = "\r\n";

HttpResponse *http_response_alloc() {
  HttpResponse *response =
      (HttpResponse *)malloc(sizeof(HttpResponse) * 1);
  if (response) {
    response->headers = malloc(sizeof(char *) * MAX_HEADERS);
    for (int i = 0; i < MAX_HEADERS; i++) {
      response->headers[i] = NULL;
    }

    response->body = NULL;
  }
  return response;
}

int http_response_add_header(HttpResponse *response, char *key,
                             char *value) {
  int i = 0;
  while (response->headers[i] != (void *)0) {
    i++;
  };

  char *sep = ": ";
  size_t hlen = strlen(key) + strlen(sep) + strlen(value) + 1;
  char *header = malloc(sizeof(char) * hlen);
  if (header) {
    // conheader
    snprintf(header, hlen, "%s%s%s", key, sep, value);

    // copy to headers array
    response->headers[i] = malloc(sizeof(char *) * hlen);
    strncpy(response->headers[i], header, hlen);

    free(header);
    return 0;
  }

  return 1;
}

size_t http_response_calc_header_size(HttpResponse *res) {
  int i = 0;
  size_t size = 0;
  while (res->headers[i] != NULL) {
    size += strlen(res->headers[i]) + strlen(clrf);
    i++;
  }

  return size;
}

size_t http_response_size(HttpResponse *res) {
  size_t bufsize = MAX_RES_LINE + http_response_calc_header_size(res);
  if (res->body != NULL) {
    bufsize += strlen(clrf) + res->body_len;
  }
  return bufsize;
}

size_t http_response_to_str(HttpResponse *res, char **resbuf) {
  int offset = 0;
  size_t bufsize = http_response_size(res);

  char *tempbuf = malloc(sizeof(char) * bufsize);
  memset(tempbuf, 0, bufsize);

  // append response line
  char res_line[MAX_RES_LINE] = {0};
  snprintf(res_line, MAX_RES_LINE, "%s %d %s\r\n", HTTP_VERSION, res->status,
           res->status_msg);
  strncat(tempbuf, res_line, strlen(res_line));
  offset += strlen(res_line);

  // append headers
  int i = 0;
  while (res->headers[i] != NULL) {
    char *header = res->headers[i];
    strncat(tempbuf, header, strlen(header));
    strncat(tempbuf, clrf, strlen(clrf));
    offset += strlen(header) + strlen(clrf);
    i++;
  }

  // append body
  if (res->body != NULL) {
    strncat(tempbuf, clrf, strlen(clrf));
    offset += strlen(clrf);
    memcpy(tempbuf + offset, res->body, res->body_len);
  }

  *resbuf = tempbuf;

  return bufsize;
}

int http_response_file_to_body(HttpResponse *res, char *path) {
  if (access(path, R_OK) != 0)
    return 1;

  FILE *stream;

  if ((stream = fopen(path, "rb")) == NULL) {
    return 2;
  } else {
    // get file size to determine response buff size
    fseek(stream, 0, SEEK_END);
    res->body_len = ftell(stream);
    fseek(stream, 0, SEEK_SET);

    res->body = malloc(sizeof(unsigned char) * res->body_len);
    memset(res->body, 0, res->body_len);

    int offset = 0;
    size_t bytes_left = res->body_len;
    unsigned long nread = 0;
    unsigned char buf[MAX_READ_BUF] = {0};
    while ((nread = fread(buf, 1, MAX_READ_BUF, stream))) {

      size_t max = MAX_READ_BUF > bytes_left ? bytes_left : MAX_READ_BUF;

      memcpy(res->body + offset, buf, max);

      offset += max;
      bytes_left -= max;
    }
  }

  fclose(stream);

  return 0;
}

void http_response_free(HttpResponse *response) {
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

Method method_supported(char *method) {
  if (strncmp(method, "GET", strlen("GET")) == 0) {
    return GET;
  } else if (strncmp(method, "HEAD", strlen("HEAD")) == 0) {
    return HEAD;
  } else {
    return UNSUPPORTED;
  }
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
