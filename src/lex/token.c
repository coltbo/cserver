#include "token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INIT_TOKENS 100
#define ALLOC_STEP 100

Token *alloc_token(TokenType type, char *lexeme) {
  Token *token = (Token *)malloc(sizeof(Token));
  if (token != NULL) {
    token->type = type;
    token->lexeme = lexeme;
  }
  return token;
}

void free_token(Token *token) {
  free(token->lexeme);
  token->lexeme = NULL;
  free(token);
  token = NULL;
}

Token *pop_token(TokenArray *tarray) {
  if (tarray->index == 0) {
    return NULL;
  }
  Token *tmp_token = tarray->tokens[tarray->index];
  tarray->tokens[tarray->index] = 0;
  return tmp_token;
}

void push_token(TokenArray *tarray, Token *token) {
  if (tarray->index == tarray->length) { // need to reallocate
    tarray->tokens =
        realloc(tarray->tokens, (ALLOC_STEP + tarray->length) * sizeof(Token));
    if (tarray->tokens == NULL) {
      // TODO - handle error
      fprintf(stderr, "Failed to realloc tokens\n");
      return;
    }
    tarray->length += ALLOC_STEP;
  }

  tarray->tokens[tarray->index++] = token;
}

Token *peek_token(TokenArray *tarray) {
  if (tarray->index == 0) {
    return NULL;
  }

  return tarray->tokens[tarray->index - 1];
}

TokenArray *alloc_token_array() {
  TokenArray *tarray = (TokenArray *)malloc(sizeof(TokenArray));
  if (tarray != NULL) {
    tarray->tokens = (Token **)malloc(sizeof(Token) * INIT_TOKENS);
    tarray->index = 0;
    tarray->length = INIT_TOKENS;
  }
  return tarray;
}

Token *find_token_by_type(TokenArray *tarray, TokenType type) {
  for (int i = 0; i < tarray->index; i++) {
    if (tarray->tokens[i]->type == type) {
      return tarray->tokens[i];
    }
  }

  return NULL;
}

Token *find_value_token_for_header(TokenArray *tarray, char *header) {
  for (int i = 0; i < tarray->index; i++) {
    char *cur_lexeme = tarray->tokens[i]->lexeme;
    if (strncmp(cur_lexeme, header, strlen(cur_lexeme)) == 0) {
      // jump to value from header token (e.g., "Connection" -> ":" ->
      // "keep-alive")
      return tarray->tokens[i + 2];
    }
  }

  return NULL;
};

void free_token_array(TokenArray *tarray) {
  for (int i = 0; i < tarray->index; i++) {
    free_token(tarray->tokens[i]);
    tarray->tokens[i] = NULL;
  }
  free(tarray->tokens);
  tarray->tokens = NULL;

  free(tarray);
  tarray = NULL;
}
