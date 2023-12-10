#include "hashtab.h"
#include <stdio.h>
#include <stdlib.h>

#define HASH_TABLE_SIZE 811

struct KeyValuePair *hashtab[HASH_TABLE_SIZE] = {0};

int hash_key(int key) { return key % HASH_TABLE_SIZE; }

struct KeyValuePair *alloc_kv_pair(int key, char *value) {
  struct KeyValuePair *kv = malloc(sizeof(struct KeyValuePair));
  if (kv != NULL) {
    kv->key = key;
    kv->value = value;
  }

  return kv;
}

int hashtab_search(int key, struct KeyValuePair *item) {
  int hkey;
  if ((hkey = hash_key(key)) > HASH_TABLE_SIZE)
    return 1;

  if (hashtab[hkey] == NULL)
    return 2;

  *item = *hashtab[hkey];
  return 0;
}

int hashtab_insert(int key, char *value) {
  int hkey;
  if ((hkey = hash_key(key)) > HASH_TABLE_SIZE)
    return 1;

  struct KeyValuePair *kv;
  if ((kv = alloc_kv_pair(key, value)) == NULL)
    return 2;

  hashtab[hkey] = kv;
  return 0;
}

int hashtab_delete(int key, struct KeyValuePair *item) {
  int hkey;
  if ((hkey = hash_key(key)) > HASH_TABLE_SIZE)
    return 1;

  if (item == NULL)
    return 2;

  *item = *hashtab[hkey];
  free(hashtab[hkey]);
  hashtab[hkey] = NULL;
  return 0;
}

void hashtab_clear() {
  for (int i = 0; i < HASH_TABLE_SIZE; i++) {
    if (hashtab[i] != NULL) {
      free(hashtab[i]);
    }
  }
}
