#ifndef TOKEN_H
#define TOKEN_H
#include <stddef.h>
enum TokenType {
  METHOD,
  URI,
  VERSION,
  HEADER,
  VALUE,
  NUMBER,
  COLON
};


struct Token {
  enum TokenType type;
  char *lexeme;
};

struct TokenArray {
  struct Token **tokens;
  size_t index;
  size_t length;
};

struct Token *alloc_token(enum TokenType type, char *lexeme);
void free_token(struct Token *token);
void push_token(struct TokenArray* tokens, struct Token *token);
struct Token *pop_token(struct TokenArray* tokens);
struct Token *peek_token(struct TokenArray* tokens);
struct TokenArray *alloc_token_array();
void free_token_array(struct TokenArray *tokens);
#endif
