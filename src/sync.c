
#include "remotestorage-fuse.h"

/* void sync(const char *path) { */
/*   log_msg("SYNC(\"%s\")", path); */
/*   struct rs_node *local_node = rs_get_node(path); */
/*   struct rs_node *remote_node = get_node_remote(path); */
/*   log_msg("Got local node: %s", inspect_rs_node(local_node)); */
/*   log_msg("Got remote node: %s", inspect_rs_node(remote_node)); */
/*   if(remote_node) { */
/*     if(local_node) { */
/*       free_node(local_node); */
/*     } */

/*     if(remote_node->is_dir) { */
/*       struct rs_dir_entry *entry; */
/*       char *child_path; */
/*       int path_len = strlen(path); */
/*       for(entry = (struct rs_dir_entry*)remote_node->data; */
/*           entry != NULL; */
/*           entry = entry->next) { */
/*         child_path = xmalloc(path_len + strlen(entry->name) + 1); */
/*         sprintf(child_path, "%s%s", path, entry->name); */
/*         sync(child_path); */
/*         free(child_path); */
/*       } */
/*     } */
/*     rs_set_node(path, remote_node->data, remote_node->size, remote_node->rev, remote_node->is_dir); */
/*     free_node(remote_node); */
/*   } */
/* } */
