
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
  .flush = rs_flush,
  .create = rs_create
};

static struct fuse_opt rs_opts[] = {
  { "base_url=%s", offsetof(struct rs_config, base_url), 0 },
  { "token=%s", offsetof(struct rs_config, token), 0 }
};

#define REQUIRE_OPTION(key) if(! RS_CONFIG. key) {                      \
    fprintf(stderr, "ERROR: Required option '" __STRING(key) "' not given!\n"); \
    abort();                                                            \
  }

void cleanup() {
  cleanup_remote();
  free(RS_CONFIG.base_url);
  free(RS_CONFIG.token);
  free(RS_CONFIG.auth_header);
}

int main(int argc, char **argv)
{
  open_log();

  char *d = xmalloc(100);
  time_t t = time(NULL);
  struct tm *tmp = localtime(&t);
  strftime(d, 99, "%c", tmp);

  log_msg("BEGIN LOG rs-mount %s", d);

  free(d);
  
  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
  memset(&RS_CONFIG, 0, sizeof(struct rs_config));
  fuse_opt_parse(&args, &RS_CONFIG, rs_opts, NULL);

  REQUIRE_OPTION(base_url);
  REQUIRE_OPTION(token);

  RS_CONFIG.base_url_len = strlen(RS_CONFIG.base_url);
  RS_CONFIG.auth_header = xmalloc(strlen(RS_CONFIG.token) + 23);
  sprintf(RS_CONFIG.auth_header, "Authorization: Bearer %s", RS_CONFIG.token);

  curl_global_init(CURL_GLOBAL_ALL);
  init_remote();

  atexit(cleanup);

  return fuse_main(args.argc, args.argv, &rs_ops, NULL);
}



  /* rs_init_cache(); */

  /* sync("/"); */

  /* rs_set_node("/foo", "bar", 3, "1", false); */
  /* rs_set_node("/what/the/dir", "", 0, "2", false); */

  /* struct rs_node *foo = rs_get_node("/foo"); */
  /* if(foo){ */
  /*   fprintf(stderr, "foo => %s\n", foo->data); */
  /* } */
  /* foo = rs_get_node("/what/the/"); */
  /* if(foo){ */
  /*   printf("0x%x: is_dir: %s\n", foo, foo->is_dir ? "yes" : "no"); */
  /* }else{ */
  /*   fprintf(stderr, "/what/the is missing\n"); */
  /* } */
  /* struct rs_node *root = rs_get_node("/"); */

  /* printf("0x%x: is_dir: %s\n", root, root->is_dir ? "yes" : "no"); */

  /* if(root->is_dir) { */
  /*   struct rs_dir_entry *entry; */
  /*   for(entry = (struct rs_dir_entry*)root->data; entry != NULL; entry = entry->next) { */
  /*     printf("ENTRY: %s\n", entry->name); */
  /*   } */
  /* } */
