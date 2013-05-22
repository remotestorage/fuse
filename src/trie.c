
#include "remotestorage-fuse.h"

#include <assert.h>

typedef struct trie_node {
  char key; // key == 0 represents end.
  char *childkeys;
  struct trie_node **children;
  void *value;
} TrieNode;

TrieNode *new_node(char key, void *value) {
  TrieNode *node = xmalloc(sizeof(TrieNode));

  node->key = key;
  node->value = value;

  node->childkeys = xmalloc(1);
  *node->childkeys = 0;
  node->children = xmalloc(1); // alloc, so realloc (in append) has a reference.
  return node;
}

TrieNode *new_trie() {
  return new_node(0, NULL);
}

void append(TrieNode *parent, TrieNode *child) {
  int len = strlen(parent->childkeys);
  parent->children = xrealloc(parent->children, (len + 1) * sizeof(TrieNode*));
  parent->childkeys = xrealloc(parent->childkeys, len + 1);
  parent->children[len] = child;
  parent->childkeys[len] = child->key;
  parent->childkeys[++len] = 0;
}

TrieNode *find_child(TrieNode *node, char key) {
  char c;
  for(int i = 0; (c = node->childkeys[i]) != 0; i++) {
    if(c == key) {
      return node->children[i];
    }
  }
  return NULL;
}

void trie_insert(TrieNode *parent, const char *key, void *value) {
  fprintf(stderr, "trie_insert(0x%x, %s, 0x%x)\n", parent, key, value);
  TrieNode *child;
  if(*key) {
    child = find_child(parent, *key);
    if(! child) {
      child = new_node(*key, NULL);
      append(parent, child);
    }
    trie_insert(child, ++key, value);
  } else {
    parent->value = value;
  }
}

void *trie_search(TrieNode *parent, const char *key) {
  fprintf(stderr, "trie_search(0x%x, %s)\n", parent, key);
  TrieNode *child;
  if(*key) {
    child = find_child(parent, *key);
    if(child) {
      return trie_search(child, ++key);
    } else {
      return NULL;
    }
  } else {
    return parent->value;
  }
}

#ifdef DEBUG_TRIE

void _dump_tree(TrieNode *node, int depth) {
  for(int d=0;d<depth;d++) {
    fprintf(stderr, " ");
  }
  if(node->key) {
    fprintf(stderr, "%c", node->key);
    if(node->value) {
      fprintf(stderr, " -> 0x%x", node->value);
    }
    fprintf(stderr, "\n", node->key);
  }
  int len = strlen(node->childkeys);
  depth++;
  for(int i=0;i<len;i++) {
    _dump_tree(node->children[i], depth);
  }
}

void dump_tree(TrieNode *root) {
  _dump_tree(root, 0);
}

int main(int argc, char **argv) {
  TrieNode *root = new_node(0, NULL);
  insert(root, "/foo/bar", "baz");
  insert(root, "/foo/blubb", "bla");
  insert(root, "/asdf/dassf/fdas", "blubb");
  insert(root, "/asdd/f/dsa/s", "fasd");

  dump_tree(root);

  char *result = (char*)search(root, "/asdf/dassf/fdas");

  fprintf(stderr, "RESULT: %s\n", result);
  return 0;
}

#endif
