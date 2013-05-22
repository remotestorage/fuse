
#include "remotestorage-fuse.h"

struct rs_node *make_node(const char *path) {
  struct rs_node *node = xmalloc(sizeof(struct rs_node));
  memset(node, 0, sizeof(struct rs_node));
  char *tmp = strdup(path);
  node->name = strdup(basename(tmp));
  free(tmp);
  return node;
}

size_t write_body(char *ptr, size_t size, size_t nmemb, void *userdata) {
  struct rs_node *node = (struct rs_node*) userdata;
  off_t prev_size = node->size;
  node->size += size * nmemb;
  if(! node->data) {
    node->data = xmalloc(node->size);
  } else {
    node->data = xrealloc(node->data, node->size);
  }
  memcpy(node->data + prev_size, ptr, size * nmemb);
  return size;
}

TrieNode *trie_root;

void rs_init_cache() {
  trie_root = new_trie();
}

char *strip_slash(const char *path) {
  char *clean_path = path;
  int len = strlen(path);
  if(len == 1) {
    return path;
  }
  if(path[len - 1] == '/') {
    clean_path = strdup(path);
    clean_path[len - 1] = 0;
  }
  return clean_path;
}

struct rs_node *rs_get_node(const char *path) {
  char *clean_path = strip_slash(path);
  struct rs_node *node = (struct rs_node*) trie_search(trie_root, clean_path);
  if(path != clean_path) {
    free(clean_path);
  }
  return node;
}

void rs_update_dir(const char *parent_path,  char *child_name, char* rev) {
  fprintf(stderr, "rs_update_dir # parent_path : %s\tchild_name : %s\n", parent_path, child_name);
  
  struct rs_node *parent_node = rs_get_node(parent_path);
  fprintf(stderr, "got parent node: 0x%x\n", parent_node);
  struct rs_dir_entry *child_entry = xmalloc(sizeof(struct rs_dir_entry));
  child_entry->name = child_name;
  child_entry->rev = rev;
  if(parent_node) {
    child_entry->next = parent_node->data;
  }
  rs_set_node(parent_path, child_entry, sizeof(struct rs_dir_entry), rev, true);
}

void rs_set_node(const char *path, void *data, off_t size, char *rev, bool is_dir) {
  char *clean_path = strip_slash(path);

  fprintf(stderr, "rs_set_node # clean_path : %s, rev : %s\n", clean_path, rev);
  struct rs_node *node = make_node(clean_path);
  char *tmp = strdup(clean_path);
  char *dname = strdup(dirname(tmp));
  free(tmp);

  node->size = size;
  node->data = data;
  node->rev = strdup(rev);
  node->is_dir = is_dir;
  trie_insert(trie_root, clean_path, (void*)node);


  printf("NODE DATA %x\n", node->data);

  fprintf(stderr, "dirname of %s is %s\n", clean_path, dname);

  if(strcmp(dname, "/") != 0) {
    // append trailing slash to dirname
    int dnamelen = strlen(dname);
    dname = realloc(dname, dnamelen + 2);
    dname[dnamelen++] = '/';
    dname[dnamelen] = 0;
  }

  log_msg("SET NODE %s", inspect_rs_node(node));

  printf("NODE DATA %x\n", node->data);

  if(strcmp(clean_path, "/") != 0) {
    rs_update_dir(dname, node->name, rev);
  }

  if(clean_path != path) {
    free(clean_path);
  }
}

char *inspect_rs_node(struct rs_node *node) {
  static char s[4096];
  snprintf(s, sizeof(s),
           "<Node \"%s\" is_dir=%d size=%d",
           node->name, node->is_dir, node->size);
  // work around snprintf bug that made it impossible
  // to output anything but 0 in the above call, after writing
  // size=%d (glibc 2.13)
  int tmplen = strlen(s);
  snprintf(s + tmplen, sizeof(s) - tmplen,
           " data=0x%x>", node->data);
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
