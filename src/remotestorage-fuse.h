
#ifndef _REMOTESTORAGE_FUSE_H_
#define _REMOTESTORAGE_FUSE_H_

#include <errno.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <libgen.h>

#include <curl/curl.h>

#define FUSE_USE_VERSION 26

#include <fuse.h>

static void *xmalloc(size_t count) {
  void *ptr = malloc(count);
  if(! ptr) {
    perror("malloc() failed");
    abort();
  }
  memset(ptr, 0, count);
  return ptr;
}

static void *xrealloc(void *inptr, size_t count) {
  void *ptr = realloc(inptr, count);
  if(! ptr) {
    perror("realloc() failed");
    abort();
  }
  return ptr;
}

/* FUSE operations (operations.c) */

int rs_getattr(const char *path, struct stat *stbuf);
int rs_truncate(const char *path, off_t length);
int rs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
               struct fuse_file_info *fi);
int rs_open(const char *path, struct fuse_file_info *fi);
int rs_read(const char *path, char *buf, size_t size, off_t offset,
            struct fuse_file_info *fi);
int rs_write(const char *path, const char *buf, size_t size, off_t offset,
             struct fuse_file_info *fi);
int rs_mkdir(const char *path, mode_t mode);
int rs_rmdir(const char *path);
int rs_flush(const char *path, struct fuse_file_info *fi);
int rs_create(const char *path, mode_t mode, struct fuse_file_info *fi);
int rs_unlink(const char *path);

/* clutter */

typedef enum { false = 0, true = 1 } bool;

/* remotestorage types */

struct rs_config {
  // actual options
  char *base_url;
  char *token;

  // computed attributes
  size_t base_url_len;
  char *auth_header;
} RS_CONFIG;

struct rs_dir_entry {
  char *name;
  char *rev;
  struct rs_dir_entry *next;
  bool is_dir;
};

struct rs_node {
  char *name;
  bool is_dir;
  off_t size;
  char *data;
  char *rev;
};

/* helpers */

void parse_listing(struct rs_node *node);
void open_log();
void log_msg(char *format, ...);
char *adjust_path(const char *path, bool is_dir);
bool is_dir(const char* path);
char *rs_dirname(const char *path);
char *rs_basename(const char *path);
char *strip_slash(const char *path);

/* node-store operations */

void rs_init_cache();
struct rs_node *rs_get_node(const char *path);
void rs_set_node(const char *path, void *data, off_t size, char *rev, bool is_dir);
char *inspect_rs_node(struct rs_node *node);
char *inspect_rs_dir_entry(struct rs_dir_entry *entry);
struct rs_node *make_node(const char *path);
void free_node(struct rs_node *node);

/* void sync(const char *path); */

/* remote operations */

void init_remote();
void cleanup_remote();
struct rs_node *get_node_remote(const char *path, bool fetch_body);
struct rs_node *get_node_remote_via_parent(const char *path, bool fetch_body);
int put_node_remote(const char *path, struct rs_node *node);
int delete_node_remote(const char *path);

/* TRIE */

typedef struct trie_node TrieNode;
TrieNode *new_trie();
void trie_insert(TrieNode *parent, const char *key, void *value);
void *trie_search(TrieNode *parent, const char *key);

#endif
