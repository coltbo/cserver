#include "server.h"
#include "lex/scan.h"
#include "lex/token.h"
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_REQUEST 8192
#define MAX_RES_LINE 100
#define HTTP_VERSION "HTTP/1.1"

int serverfd;

void handler(int signo, siginfo_t *info, void *context) {
  printf("[info] closing sockets...\n");
  close(serverfd);
  printf("[info] sockets closed\n");
}

void handle_request(int clientfd);

/*
void init_resp_codes() {
  hashtab_insert(SWITCH_PROT, "Switching Protocols");
  hashtab_insert(PROCCESSING, "Processing");
  hashtab_insert(EARLY_HINTS, "Early Hints");

  // hashtab_insert(OK, "OK");
  hashtab_insert(CREATED, "Created");
  hashtab_insert(ACCEPTED, "Accepted");
  hashtab_insert(NO_CONTENT, "No Content");

  hashtab_insert(MULTIPLE_CHOICE, "Multiple Choices");
  hashtab_insert(MOVED_PERMANENTLY, "Moved Permanently");
  hashtab_insert(FOUND, "Found");
  hashtab_insert(NOT_MODIFIED, "Not Modified");
  hashtab_insert(TEMP_REDIRECT, "Temporary Redirect");
  hashtab_insert(PERM_REDIRECT, "Permanent Redirect");

  hashtab_insert(BAD_REQUEST, "Bad Request");
  hashtab_insert(BAD_REQUEST, "Bad Request");
  hashtab_insert(UNAUTHORIZED, "Unauthorized");
  hashtab_insert(FORBIDDEN, "Forbidden");
  hashtab_insert(NOT_FOUND, "Not Found");
  hashtab_insert(NOT_ALLOWED, "Method Not Allowed");
  hashtab_insert(NOT_ACCEPTABLE, "Not Acceptable");
  hashtab_insert(TIMEOUT, "Request Timeout");
  hashtab_insert(LEN_REQUIRED, "Length Required");
  hashtab_insert(TOO_LARGE, "Payload Too Large");
  hashtab_insert(TOO_LONG, "URI Too Long");
  hashtab_insert(UNS_MEDIA_TYPE, "Unsupported Media Type");
  hashtab_insert(TEAPOT, "I'm a teapot");
  hashtab_insert(TOO_MANY, "Too Many Requests");

  hashtab_insert(SERVER_ERROR, "Internal Server Error");
  hashtab_insert(NOT_IMPLEMENTED, "Not Implemented");
  hashtab_insert(BAD_GATEWAY, "Bad Gateway");
  hashtab_insert(UNAVAILABLE, "Service Unavailable");
  hashtab_insert(GATEWAY_TIMEOUT, "Gateway Timeout");
  hashtab_insert(NOT_SUPPORTED, "HTTP Version Not Supported");
}
*/

int server_run(int port) {
  // init_resp_codes();
  struct sockaddr_in address;
  socklen_t addrlen = sizeof(address);

  struct sigaction act = {0};

  act.sa_flags = SA_SIGINFO;
  act.sa_sigaction = &handler;
  if (sigaction(SIGINT, &act, NULL) == -1) {
    perror("sigaction");
    return 1;
  }

  if ((serverfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket failed");
    return 1;
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  /* bind the address and port number to a socket */
  if (bind(serverfd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    return 2;
  }

  printf("[info] listening for requests on %d!\n", port);

  for (;;) {
    /* waiting for client to initiate a connection. The second parameter
       specifies the allowed backlog of requests on the socket.*/
    if (listen(serverfd, 3) < 0) {
      perror("listen");
      return 3;
    }

    int clientfd;
    /* takes the first connection on the socket queue and creates a new
       connected socket with the client.*/
    if ((clientfd = accept(serverfd, (struct sockaddr *)&address, &addrlen)) <
        0) {
      perror("accept");
      return 4;
    }

    handle_request(clientfd);
  }
}

void handle_request(int clientfd) {
  char request[MAX_REQUEST] = {0};
  ssize_t valread = read(clientfd, request, MAX_REQUEST);

  struct TokenArray *tarray = scan(request);

  struct Token *method_token = find_token_by_type(tarray, METHOD);
  if (method_token == NULL) {
    fprintf(stderr, "[error] no method passed in request\n");
    return;
  }

  struct Token *uri_token = find_token_by_type(tarray, URI);
  if (uri_token == NULL) {
    fprintf(stderr, "[error] no uri passed in request\n");
    return;
  }

  struct Token *version_token = find_token_by_type(tarray, VERSION);
  if (version_token == NULL) {
    fprintf(stderr, "[error] no HTTP version passed in request\n");
    return;
  }

  printf("[info] handling %s request for uri %s\n", method_token->lexeme,
         uri_token->lexeme);

  // TODO - maybe I should have some configuration
  // for the root web directory?
  char *web_root = "/home/colten/projects/cserver/www";
  char *uri =
      (strcmp(uri_token->lexeme, "/") == 0) ? "/index.html" : uri_token->lexeme;
  int uri_len = strlen(web_root) + strlen(uri) + 1;

  char req_path[uri_len];
  snprintf(req_path, uri_len, "%s%s", web_root, uri);

  /* vars used for response */
  char response_line[MAX_RES_LINE] = {0};

  if (access(req_path, R_OK) == 0) {
    FILE *stream;

    if ((stream = fopen(req_path, "r")) == NULL) {
      // TODO - handle server errors
      fprintf(stderr, "[error] could not open requested file\n");
    } else {
      // TODO - need to actually generate a http response

      // Send status line
      snprintf(response_line, MAX_RES_LINE, "%s %s\r\n", HTTP_VERSION, OK);
      send(clientfd, response_line, strlen(response_line), 0);

      // send content
      while (fgets(response_line, MAX_RES_LINE, stream)) {
        send(clientfd, response_line, strlen(response_line), 0);
      }
    }

    fclose(stream);
  } else {
    snprintf(response_line, MAX_RES_LINE, "%s %s\r\n", HTTP_VERSION, NOT_FOUND);
    send(clientfd, response_line, strlen(response_line), 0);
  }

  free_token_array(tarray);

  close(clientfd);
}
