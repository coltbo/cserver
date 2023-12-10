#ifndef SERVER_H
#define SERVER_H

enum StatusCode {
  /* information responses */
  CONTINUE = 100,
  SWITCH_PROT = 101,
  PROCCESSING = 102,
  EARLY_HINTS = 103,

  /* successful responses */
  OK = 200,
  CREATED = 201,
  ACCEPTED = 202,
  NO_CONTENT = 204,

  /* redirection messages */
  MULTIPLE_CHOICE = 300,
  MOVED_PERMANENTLY = 301,
  FOUND = 302,
  NOT_MODIFIED = 304,
  TEMP_REDIRECT = 307,
  PERM_REDIRECT = 308,

  /* client error responses */
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

  /* server error responses */
  SERVER_ERROR = 500,
  NOT_IMPLEMENTED = 501,
  BAD_GATEWAY = 502,
  UNAVAILABLE = 503,
  GATEWAY_TIMEOUT = 504,
  NOT_SUPPORTED = 505,
};

int server_run(int port);

#endif
