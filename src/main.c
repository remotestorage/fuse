
#include "remotestorage-fuse.h"

static struct fuse_operations rs_ops = {
  .getattr = rs_getattr,
  .readdir = rs_readdir,
  .open = rs_open,
  .read = rs_read,
  .write = rs_write,
  .truncate = rs_truncate,
  .mkdir = rs_mkdir,
  .rmdir = rs_rmdir,
  .flush = rs_flush
};

static struct fuse_opt rs_opts[] = {
  { "base_url=%s", offsetof(struct rs_config, base_url), 0 },
  { "token=%s", offsetof(struct rs_config, token), 0 }
};

#define REQUIRE_OPTION(key) if(! RS_CONFIG. key) {                      \
    fprintf(stderr, "ERROR: Required option '" __STRING(key) "' not given!\n"); \
    abort();                                                            \
  }

int main(int argc, char **argv)
{
  open_log();
  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
  memset(&RS_CONFIG, 0, sizeof(struct rs_config));
  fuse_opt_parse(&args, &RS_CONFIG, rs_opts, NULL);

  REQUIRE_OPTION(base_url);
  REQUIRE_OPTION(token);

  RS_CONFIG.base_url_len = strlen(RS_CONFIG.base_url);
  RS_CONFIG.auth_header = xmalloc(strlen(RS_CONFIG.token) + 23);
  sprintf(RS_CONFIG.auth_header, "Authorization: Bearer %s", RS_CONFIG.token);

  curl_global_init(CURL_GLOBAL_ALL);

  rs_init_cache();

  rs_set_node("/foo", "bar", 3);

  struct rs_node *root = rs_get_node("/");

  printf("is_dir: %s\n", root->is_dir ? "yes" : "no");

  if(root->is_dir) {
    struct rs_node *entry;
    for(entry = root->children; entry != NULL; entry = entry->next) {
      printf("ENTRY: %s\n", entry->name);
    }
  }

  /* return fuse_main(args.argc, args.argv, &rs_ops, NULL); */
}
