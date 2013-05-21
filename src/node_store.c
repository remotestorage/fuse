
#include "remotestorage-fuse.h"

struct rs_node *make_node(char *name) {
  struct rs_node *node = xmalloc(sizeof(struct rs_node));
  memset(node, 0, sizeof(struct rs_node));
  node->name = strdup(name);
  return node;
}

size_t write_body(char *ptr, size_t size, size_t nmemb, void *userdata) {
  struct rs_node *node = (struct rs_node*) userdata;
  off_t prev_size = node->size;
  node->size += size * nmemb;
  if(! node->data) {
    node->data = xmalloc(node->size + 1);
  } else {
    node->data = xrealloc(node->data, node->size + 1);
  }
  memcpy(node->data + prev_size, ptr, size * nmemb);
  node->data[node->size] = 0;
  return size;
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

struct rs_node *cache_root;

void rs_init_cache() {
  cache_root = xmalloc(sizeof(struct rs_node));
  cache_root->is_dir = true;
}

struct rs_node *find_child(struct rs_node *node, const char *name) {
  log_msg("find_child(%s) from %s", name, node->name);
  struct rs_node *child;
  for(child = node->children; child != NULL; child = child->next) {
    if(strcmp(child->name, name) == 0) {
      return child;
    }
  }
  return NULL;
}

void add_child(struct rs_node *parent, struct rs_node *child) {
  struct rs_node *next = parent->children;
  parent->children = child;
  child->next = next;
}

struct rs_node *rs_get_node(const char *_path) {
  char *tok_save = NULL;

  char *path = strdup(_path);

  struct rs_node *node = cache_root;

  char *current;
  for(current = strtok_r(path, "/", &tok_save);
      current != NULL;
      current = strtok_r(NULL, "/", &tok_save)) {
    node = find_child(node, current);
    if(! node) {
      return NULL;
    }
  }

  free(path);

  return node;
}

void rs_set_node(const char *_path, char *data, off_t size) {
  char *tok_save = NULL;

  char *path = strdup(_path);

  struct rs_node *node = cache_root, *prev = NULL;
  char *current;
  for(current = strtok_r(path, "/", &tok_save);
      current != NULL;
      current = strtok_r(NULL, "/", &tok_save)) {
    prev = node;
    node = find_child(prev, current);
    if(! node) {
      node = make_node(current);
      add_child(prev, node);
    }
  }

  node->data = xmalloc(size + 1);
  node->size = size;
  memcpy(node->data, data, size);
  node->data[node->size] = 0;

  free(path);
}
