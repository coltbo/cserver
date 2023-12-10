#ifndef HASHTAB_H
#define HASHTAB_H

struct KeyValuePair {
  int key;
  char *value;
};

int hashtab_search(int key, struct KeyValuePair *item);
int hashtab_insert(int key, char *value);
int hashtab_delete(int key, struct KeyValuePair *item);
void hashtab_clear();

#endif
