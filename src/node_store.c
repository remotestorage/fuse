
#include "remotestorage-fuse.h"

struct rs_node *make_node(const char *path) {
  struct rs_node *node = xmalloc(sizeof(struct rs_node));
  memset(node, 0, sizeof(struct rs_node));
  node->name = rs_basename(path);
  node->is_dir = is_dir(node->name);
  return node;
}

void free_dir_entry(struct rs_dir_entry *entry) {
  if(entry == NULL) {
    return;
  }
  struct rs_dir_entry *next = entry->next;
  free(entry->name);
  free(entry->rev);
  free(entry);
  free_dir_entry(next);
}

void free_node(struct rs_node *node) {
  free(node->name);
  free(node->rev);
  if(node->is_dir) {
    free_dir_entry((struct rs_dir_entry*)node->data);
  } else if(node->data) {
    free(node->data);
  }
  free(node);
}

TrieNode *trie_root;

void rs_init_cache() {
  trie_root = new_trie();
}

struct rs_node *rs_get_node(const char *path) {
  struct rs_node *node = (struct rs_node*) trie_search(trie_root, path);
  return node;
}

void rs_update_dir(const char *parent_path,  char *child_name, char* rev) {
  fprintf(stderr, "rs_update_dir # parent_path : %s\tchild_name : %s\n", parent_path, child_name);
  
  struct rs_node *parent_node = rs_get_node(parent_path);
  fprintf(stderr, "got parent node: 0x%x\n", parent_node);
  struct rs_dir_entry *child_entry = xmalloc(sizeof(struct rs_dir_entry));
  child_entry->name = child_name;
  child_entry->rev = rev;
  child_entry->is_dir = is_dir(child_name);
  if(parent_node) {
    child_entry->next = (struct rs_dir_entry*)parent_node->data;
  }
  rs_set_node(parent_path, child_entry, sizeof(struct rs_dir_entry), rev, true);
}

void rs_set_node(const char *path, void *data, off_t size, char *rev, bool is_dir) {
  char *clean_path = adjust_path(path, is_dir);

  fprintf(stderr, "rs_set_node # clean_path : %s, rev : %s\n", clean_path, rev);
  struct rs_node *node = make_node(clean_path);
  char *dname = rs_dirname(path);

  node->size = size;
  node->data = data;
  node->rev = rev ? strdup(rev) : strdup("");

  trie_insert(trie_root, clean_path, (void*)node);

  if(node->is_dir) {
    char *tmp = strip_slash(clean_path);
    trie_insert(trie_root, tmp, (void*)node);
    free(tmp);
  }

  printf("NODE DATA %x\n", node->data);

  fprintf(stderr, "dirname of %s is %s\n", clean_path, dname);

  log_msg("SET NODE %s", inspect_rs_node(node));

  printf("NODE DATA %x\n", node->data);

  if(strcmp(clean_path, "/") != 0) {
    rs_update_dir(dname, node->name, rev);
    free(dname);
  }

  if(clean_path != path) {
    free(clean_path);
  }
}

char *inspect_rs_node(struct rs_node *node) {
  static char s[4096];
  if(node) {
    snprintf(s, sizeof(s),
             "<Node \"%s\" is_dir=%d size=%d",
             node->name, node->is_dir, node->size);
    // work around snprintf bug that made it impossible
    // to output anything but 0 in the above call, after writing
    // size=%d (glibc 2.13)
    int tmplen = strlen(s);
    snprintf(s + tmplen, sizeof(s) - tmplen,
             " data=0x%x>", node->data);
  } else {
    snprintf(s, sizeof(s), "<Node (null)>");
  }
  return s;
}

char *inspect_rs_dir_entry(struct rs_dir_entry *entry) {
  static char s[4096];
  if(entry) {
    snprintf(s, sizeof(s), "<DirEntry \"%s\" next=0x%x>", entry->name, entry->next);
  } else {
    snprintf(s, sizeof(s), "<DirEntry (null)>");
  }
  return s;
}

/*
struct rs_node *rs_get_node(const char *path) {
  CURL *curl_handle = curl_easy_init();

  struct rs_node *node = xmalloc(sizeof(struct rs_node));
  memset(node, 0, sizeof(struct rs_node));

  node->is_dir = path[strlen(path) - 1] == '/' ? true : false;
  
  // set URL
  char *url = xmalloc(RS_CONFIG.base_url_len + strlen(path) + 1);
  sprintf(url, "%s%s", RS_CONFIG.base_url, path);
  curl_easy_setopt(curl_handle, CURLOPT_URL, url);
  // set headers
  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, RS_CONFIG.auth_header);
  curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, node);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_body);

  curl_easy_perform(curl_handle);

  long status;
  curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &status);

  if(status >= 400) {
    fprintf(stderr, "REQUEST FAILED. Status: %d\n", status);
    abort();
  }

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl_handle);
  free(url);

  if(node->is_dir) {
    parse_listing(node);
  }

  return node;
}
*/
