#ifndef TOKEN_H
#define TOKEN_H
#include <stddef.h>

typedef enum { METHOD, URI, VERSION, HEADER, VALUE, NUMBER, COLON } TokenType;

typedef struct {
  TokenType type;
  char *lexeme;
} Token;

typedef struct {
  Token **tokens;
  size_t index;
  size_t length;
} TokenArray;

Token *alloc_token(TokenType type, char *lexeme);
void free_token(Token *token);
void push_token(TokenArray *tarray, Token *token);
Token *pop_token(TokenArray *tarray);
Token *peek_token(TokenArray *tarray);
TokenArray *alloc_token_array();
Token *find_token_by_type(TokenArray *tarray, TokenType type);
Token *find_value_token_for_header(TokenArray *tarray, char *header);
void free_token_array(TokenArray *tarray);
#endif
