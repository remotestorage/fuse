
#include "remotestorage-fuse.h"

int rs_getattr(const char *path, struct stat *stbuf)
{
  memset(stbuf, 0, sizeof(struct stat));
  struct rs_node *node = rs_get_node(path);
  if(node) {
    if(node->is_dir) {
      //stbuf->st_mode = S_IFDIR | 0750;
      stbuf->st_mode = S_IFDIR | 0755;
      stbuf->st_nlink = 2;
    } else {
      //stbuf->st_mode = S_IFREG | 0640;
      stbuf->st_mode = S_IFREG | 0644;
      stbuf->st_nlink = 1;
      stbuf->st_size = node->size;
    }
    return 0;
  } else {
    return -ENOENT;
  }
}

int rs_truncate(const char *path, off_t length) {}

int rs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
  struct rs_node *node = rs_get_node(path);

  if(node == NULL) {
    return -ENOENT;
  }

  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);

  struct rs_dir_entry *entry;
  for(entry = (struct rs_dir_entry*)node->data; entry != NULL; entry = entry->next) {
    filler(buf, entry->name, NULL, 0);
  }
  return 0;
}

int rs_open(const char *path, struct fuse_file_info *fi) {
  struct rs_node *node = rs_get_node(path);
  return node ? 0 : -ENOENT;
}

int rs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  struct rs_node *node = rs_get_node(path);
  if(! node) {
    return -ENOENT;
  }

  log_msg("%s size %d", path, node->size);
  log_msg("%s requested %d", path, size);
  log_msg("%s offset %d", path, offset);

  if(offset + size > node->size) {
    size = node->size - offset;
  }

  log_msg("%s result size %d", path, size);

  memcpy(buf, node->data + offset, size);

  return size;
}

int rs_write(const char *path, const char *buf, size_t size, off_t offset,
             struct fuse_file_info *fi) {};

int rs_mkdir(const char *path, mode_t mode) {}

int rs_rmdir(const char *path) {}

int rs_flush(const char *path, struct fuse_file_info *fi) {}
