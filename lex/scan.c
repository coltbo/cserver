#include "scan.h"
#include "stdio.h"
#include "token.h"
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* source positioning */
int start = 0;
int current = 0;
int line = 1;

/* source data */
char *source;
size_t src_len = 0;

struct TokenArray *tarray;

/* Gets a text lexeme and appends it to the token array.
 * If the previous token is a ':', it will read the rest of
 * the line as a value. Otherwise, it will only read the next
 * set of contiguous characters.*/
void get_text();

/* Gets a number lexeme and appends it to the token array. */
void get_number();

/* Get's the string value between start and current. */
char *get_current_substr_value();

/* Checks if at end of file. */
bool is_eof() { return current >= src_len; }

/* Returns the next character without eating it. */
char peek() {
  if (is_eof())
    return '\0';
  return source[current];
}

/* Creates a lexeme string from a character.
 * A lexeme string needs to be escaped, so a single
 * character won't work.*/
char *get_lexeme_from_char(char c) {
  char *lexeme = malloc(sizeof(char) * 2);
  lexeme[0] = c;
  lexeme[1] = '\0';
  return lexeme;
}

/* Move the current position of the scanner by 1. */
char advance() { return source[current++]; }

/* Read the next token and process it. */
void scan_token() {
  char c;

  // handle request line
  if (line == 1) {
    while (peek() != '\r')
      advance();

    char *token;
    // first token is method
    if ((token = strtok(get_current_substr_value(), " ")) != NULL) {
      push_token(tarray, alloc_token(METHOD, token));
    }
    // second token is uri
    if ((token = strtok(NULL, " ")) != NULL) {
      push_token(tarray, alloc_token(URI, token));
    }
    // third token is version
    if ((token = strtok(NULL, " ")) != NULL) {
      push_token(tarray, alloc_token(VERSION, token));
    }

    line++;
    return;
  }

  switch ((c = advance())) {
  case ' ':
    break;
  case ':':
    push_token(tarray, alloc_token(COLON, get_lexeme_from_char(c)));
    break;
  case '\r':
    break;
  case '\n':
    line++;
    start = 0;
    break;
  default:
    if (isdigit(c)) {
      get_number();
    } else {
      get_text();
    }
    break;
  }

  start = current;
}

void get_text() {
  char next;

  // previous token was colon, this means we need to get the key value
  struct Token *prev_token;
  if ((prev_token = peek_token(tarray)) != NULL &&
      strcmp(prev_token->lexeme, ":") == 0) {
    while ((next = peek()) == ' ') {
      start++;
      advance();
    }

    while ((next = peek()) != '\n' && next != '\r' && !is_eof()) {
      advance();
    }

    push_token(tarray, alloc_token(VALUE, get_current_substr_value()));
  } else {
    while ((next = peek()) != ' ' && next != ':' && next != '\n' && next != '\r' && !is_eof()) {
      advance();
    }

    push_token(tarray, alloc_token(HEADER, get_current_substr_value()));
  }
}

char *get_current_substr_value() {
  int len = current - start;
  char *value = (char *)malloc(sizeof(char) * (len + 1));
  value[len] = '\0'; // need to set terminator for memcpy
  memcpy(value, source + start, len);
  return value;
}

void get_number() {
  while (isdigit(peek()) && !is_eof()) {
    advance();
  }

  push_token(tarray, alloc_token(NUMBER, get_current_substr_value()));
}

/* Get a token array from an input string. */
struct TokenArray *scan(char *input) {
  source = input;
  src_len = strlen(source);
  tarray = alloc_token_array();
  while (!is_eof()) {
    scan_token();
  }

  return tarray;
}
