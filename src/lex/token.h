#ifndef TOKEN_H
#define TOKEN_H
#include <stddef.h>
enum TokenType { METHOD, URI, VERSION, HEADER, VALUE, NUMBER, COLON };

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
void push_token(struct TokenArray *tarray, struct Token *token);
struct Token *pop_token(struct TokenArray *tarray);
struct Token *peek_token(struct TokenArray *tarray);
struct TokenArray *alloc_token_array();
struct Token *find_token_by_type(struct TokenArray *tarray,
                                 enum TokenType type);
void free_token_array(struct TokenArray *tarray);
#endif
