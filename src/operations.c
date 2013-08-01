
#include "remotestorage-fuse.h"

int rs_getattr(const char *path, struct stat *stbuf)
{
  memset(stbuf, 0, sizeof(struct stat));
  struct rs_node *node;
  if(path[0] == '/' && path[1] == 0) {
    node = get_node_remote(path, false);
  } else {
    node = get_node_remote_via_parent(path, false);
  }
  if(node) {
    if(node->is_dir) {
      log_msg("OP GETATTR %s (dir)", path);
      //stbuf->st_mode = S_IFDIR | 0750;
      stbuf->st_mode = S_IFDIR | 0755;
      stbuf->st_nlink = 2;
    } else {
      log_msg("OP GETATTR %s (file, %d bytes)", path, node->size);
      //stbuf->st_mode = S_IFREG | 0640;
      stbuf->st_mode = S_IFREG | 0644;
      stbuf->st_nlink = 1;
      stbuf->st_size = node->size;
    }
    free_node(node);
    return 0;
  } else {
    log_msg("OP GETATTR %s (ENOENT)", path);
    return -ENOENT;
  }
}

int rs_truncate(const char *path, off_t length) {
  log_msg("OP TRUNCATE %s: %d bytes", path, length);
  struct rs_node *node = get_node_remote_via_parent(path, true);
  if(! node) {
    node = make_node(path);
  }

  if(node->is_dir) {
    return -EISDIR;
  }

  int prev = node->size;
  node->size = length;
  node->data = realloc(node->data, length);
  if(prev < length) {
    memset(node->data + prev, 0, length);
  }
  
  int result = put_node_remote(path, node);

  free_node(node);
  return result == 0 ? 0 : -EIO;
}

int rs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
  char *clean_path = adjust_path(path, true);
  log_msg("OP READDIR %s", clean_path);
  //struct rs_node *node = rs_get_node(clean_path);
  struct rs_node *node = get_node_remote(clean_path, true);

  if(node == NULL) {
    if(path != clean_path) {
      free(clean_path);
    }
    return -ENOENT;
  }

  log_msg("READDIR GOT NODE: %s", inspect_rs_node(node));

  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);

  struct rs_dir_entry *entry;
  char *tmp;
  for(entry = (struct rs_dir_entry*)node->data; entry != NULL; entry = entry->next) {
    log_msg("READDIR FOUND ENTRY: %s", inspect_rs_dir_entry(entry));
    if(is_dir(entry->name)) {
      tmp = strip_slash(entry->name);
      filler(buf, tmp, NULL, 0);
      free(tmp);
    } else {
      filler(buf, entry->name, NULL, 0);
    }
  }
  if(path != clean_path) {
    free(clean_path);
  }
  free_node(node);
  return 0;
}

int rs_open(const char *path, struct fuse_file_info *fi) {
  log_msg("OP OPEN %s", path);
  struct rs_node *node = get_node_remote(path, false);
  int result = node ? 0 : -ENOENT;
  free_node(node);
  return result;
}

int rs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  log_msg("OP READ %s", path);
  struct rs_node *node = get_node_remote_via_parent(path, true);
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

  log_msg("READ RETURNING %d", size);

  free_node(node);

  return size;
}

int rs_write(const char *path, const char *buf, size_t size, off_t offset,
             struct fuse_file_info *fi) {
  log_msg("OP WRITE %s, %d bytes @%d", path, size, offset);
  struct rs_node *node = get_node_remote_via_parent(path, true);
  if(! node) {
    node = make_node(path);
  }

  if(node->is_dir) {
    return -EISDIR;
  }

  if((offset + size) > node->size) {
    node->data = realloc(node->data, offset + size);
    node->size = offset + size;
  }

  int i;
  for(i=0;i<size;i++) {
    node->data[offset + i] = buf[i];
  }

  int result = put_node_remote(path, node);

  free_node(node);
  return result == 0 ? size : -EIO;
};

int rs_mkdir(const char *path, mode_t mode) { return 0; }

int rs_rmdir(const char *path) { return 0; }

int rs_flush(const char *path, struct fuse_file_info *fi) {
  log_msg("OP FLUSH %s", path);
  return 0;
}

int rs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
  return rs_truncate(path, 0);
}

int rs_unlink(const char *path) {
  return delete_node_remote(path);
}
