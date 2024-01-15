#ifndef HTTP_H
#define HTTP_H

#include <stdbool.h>
#include <stddef.h>

#define MAX_REQUEST 8192
#define MAX_RES_LINE 100
#define MAX_HEADERS 20
#define HTTP_VERSION "HTTP/1.1"

#define GET "GET"
#define HEAD "HEAD"
#define POST "POST"

enum StatusCode {
  CONTINUE = 100,
  SWITCH_PROT = 101,
  PROCCESSING = 102,
  EARLY_HINTS = 103,

  OK = 200,
  CREATED = 201,
  ACCEPTED = 202,
  NO_CONTENT = 204,

  MULTIPLE_CHOICE = 300,
  MOVED_PERMANENTLY = 301,
  FOUND = 302,
  NOT_MODIFIED = 304,
  TEMP_REDIRECT = 307,
  PERM_REDIRECT = 308,

  BAD_REQUEST = 400,
  UNAUTHORIZED = 401,
  FORBIDDEN = 403,
  NOT_FOUND = 404,
  NOT_ALLOWED = 405,
  NOT_ACCEPTABLE = 406,
  TIMEOUT = 408,
  LEN_REQUIRED = 411,
  TOO_LARGE = 413,
  TOO_LONG = 414,
  UNS_MEDIA_TYPE = 415,
  TEAPOT = 418,
  TOO_MANY = 429,

  SERVER_ERROR = 500,
  NOT_IMPLEMENTED = 501,
  BAD_GATEWAY = 502,
  UNAVAILABLE = 503,
  GATEWAY_TIMEOUT = 504,
  NOT_SUPPORTED = 505,
};

struct HttpResponse {
  enum StatusCode status;
  long body_len;
  char *status_msg;
  unsigned char *body;
  char **headers;
};

bool is_http_version_compat(char *ver);
struct HttpResponse *http_response_alloc();
int http_response_add_header(struct HttpResponse *response, char *key, char *value);
size_t http_response_to_str(struct HttpResponse *res, char **resbuf);
void http_response_free(struct HttpResponse *response);
bool is_http_version_compat(char *ver);
bool is_method_supported(char *method);

#endif
