#ifndef SERVER_H
#define SERVER_H

/*
#define CONTINUE "100 Continue"
#define SWITCH_PROT "101 Switching Protocols"
#define PROCCESSING "102 Processing"
#define EARLY_HINTS "103 Early Hints"

#define OK "200 OK"
#define CREATED " 201 Created"
#define ACCEPTED "202 Accepted"
#define NO_CONTENT "204 No Content"

#define MULTIPLE_CHOICE "300 Multiple Choices"
#define MOVED_PERMANENTLY "301 Moved Permanently"
#define FOUND "302 Found"
#define NOT_MODIFIED "304 Not Modified"
#define TEMP_REDIRECT "307 Temporary Redirect"
#define PERM_REDIRECT "308 Permanent Redirect"

#define BAD_REQUEST "400 Bad Request"
#define UNAUTHORIZED "401 Unauthorized"
#define FORBIDDEN "403 Forbidden"
#define NOT_FOUND "404 Not Found"
#define NOT_ALLOWED "405 Method Not Allowed"
#define NOT_ACCEPTABLE "406 Not Acceptable"
#define TIMEOUT "408 Request Timeout"
#define LEN_REQUIRED "411 Length Required"
#define TOO_LARGE "413 Payload Too Large"
#define TOO_LONG "414 URI Too Long"
#define UNS_MEDIA_TYPE "415 Unsupported Media Type"
#define TEAPOT "418 I'm a teapot"
#define TOO_MANY "429 Too Many Requests"

#define SERVER_ERROR "500 Internal Server Error"
#define NOT_IMPLEMENTED "501 Not Implemented"
#define BAD_GATEWAY "502 Bad Gateway"
#define UNAVAILABLE "503 Service Unavailable"
#define GATEWAY_TIMEOUT "504 Gateway Timeout"
#define NOT_SUPPORTED "505 HTTP Version Not Supported"
*/

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

int server_run(int port);

#endif
