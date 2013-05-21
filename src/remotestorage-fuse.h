
#ifndef _REMOTESTORAGE_FUSE_H_
#define _REMOTESTORAGE_FUSE_H_

#include <errno.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

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

struct rs_node {
  char *name;
  bool is_dir;
  off_t size;
  char *data;
  char *rev;
  struct rs_node *children;
  struct rs_node *next;
};

/* helpers */

void parse_listing(struct rs_node *node);
void open_log();
void log_msg(char *format, ...);

/* node-store operations */

void rs_init_cache();
struct rs_node *rs_get_node(const char *path);
void rs_set_node(const char *path, char *data, off_t size);

#endif
