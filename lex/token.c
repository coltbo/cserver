#include "token.h"
#include <stdio.h>
#include <stdlib.h>

#define INIT_TOKENS 100
#define ALLOC_STEP 100

struct Token *alloc_token(enum TokenType type, char *lexeme) {
  struct Token *token = (struct Token *)malloc(sizeof(struct Token));
  if (token != NULL) {
    token->type = type;
    token->lexeme = lexeme;
  }
  return token;
}

void free_token(struct Token *token) {
  printf("freeing token: %s\n", token->lexeme);
  free(token->lexeme);
  token->lexeme = NULL;
  free(token);
  token = NULL;
}

struct Token *pop_token(struct TokenArray *tarray) {
  if (tarray->index == 0) {
    return NULL;
  }
  struct Token *tmp_token = tarray->tokens[tarray->index];
  tarray->tokens[tarray->index] = 0;
  return tmp_token;
}

void push_token(struct TokenArray *tarray, struct Token *token) {
  if (tarray->index == tarray->length) { // need to reallocate
    tarray->tokens = realloc(tarray->tokens, (ALLOC_STEP + tarray->length) *
                                                 sizeof(struct Token));
    if (tarray->tokens == NULL) {
      // TODO - handle error
      fprintf(stderr, "Failed to realloc tokens\n");
      return;
    }
    tarray->length += ALLOC_STEP;
  }

  tarray->tokens[tarray->index++] = token;
}

struct Token *peek_token(struct TokenArray *tarray) {
  if (tarray->index == 0) {
    return NULL;
  }

  return tarray->tokens[tarray->index - 1];
}

struct TokenArray *alloc_token_array() {
  struct TokenArray *tarray =
      (struct TokenArray *)malloc(sizeof(struct TokenArray));
  if (tarray != NULL) {
    tarray->tokens =
        (struct Token **)malloc(sizeof(struct Token) * INIT_TOKENS);
    tarray->index = 0;
    tarray->length = INIT_TOKENS;
  }
  return tarray;
}

struct Token *find_token_by_type(struct TokenArray *tarray, enum TokenType type) {
  for (int i = 0; i < tarray->index; i++) {
    if (tarray->tokens[i]->type == type) {
      return tarray->tokens[i]; 
    }
  }

  return NULL;
}

void free_token_array(struct TokenArray *tarray) {
  for (int i = 0; i < tarray->index; i++) {
    free_token(tarray->tokens[i]);
    tarray->tokens[i] = NULL;
  }
  free(tarray->tokens);
  tarray->tokens = NULL;

  free(tarray);
  tarray = NULL;
}
