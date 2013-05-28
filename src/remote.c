
#include "remotestorage-fuse.h"

CURL *curl_handle;

void init_remote() {
  curl_handle = curl_easy_init();
}

void cleanup_remote() {
  curl_easy_cleanup(curl_handle);
}

size_t write_body(char *ptr, size_t size, size_t nmemb, void *userdata) {
  char s[nmemb * size + 1];
  strncpy(s, ptr, size * nmemb);
  struct rs_node *node = (struct rs_node*) userdata;
  off_t prev_size = node->size;
  node->size += size * nmemb;
  if(! node->data) {
    node->data = xmalloc(node->size);
  } else {
    node->data = xrealloc(node->data, node->size);
  }
  memcpy(node->data + prev_size, ptr, size * nmemb);
  return size * nmemb;
}

size_t write_header(char *ptr, size_t size, size_t nmemb, void *userdata) {
  size_t bytes = size * nmemb;
  if(bytes > 16) {
    char content_length_buf[bytes - 16];
    struct rs_node *node = (struct rs_node*) userdata;
    if(strncmp(ptr, "Content-Length: ", 16) == 0) {
      strncpy(content_length_buf, ptr + 16, bytes - 16);
      node->size = atoll(content_length_buf);
    }
  }
  return bytes;
}

long perform_request(const char *path, struct rs_node* node, bool fetch_body) {
  curl_easy_reset(curl_handle);

  // set URL
  char *url = xmalloc(RS_CONFIG.base_url_len + strlen(path) + 1);
  sprintf(url, "%s%s", RS_CONFIG.base_url, path);
  curl_easy_setopt(curl_handle, CURLOPT_URL, url);
  // set headers
  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, RS_CONFIG.auth_header);
  curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);

  log_msg("[remote] %s %s", fetch_body ? "GET" : "HEAD", path);

  if(fetch_body) {
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_body);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, node);
  } else {
    curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 1);

    if(! node->is_dir) {
      curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, write_header);
      curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, node);
    }
  }

  static char errorbuf[CURL_ERROR_SIZE];
  curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, errorbuf);

  CURLcode result = curl_easy_perform(curl_handle);
  if(result != 0) {
    log_msg("curl_easy_perform() failed with status %d: %s", result, errorbuf);
    return -1;
  }
  long status = 0;
  curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &status);
  if(status > 400 && status != 404) {
    // (404 is ok, as it means ENOENT)
    log_msg("[remote] FAILED url: %s, status: %d", url, (int)status);
  } else {
    log_msg("[remote] SUCCESS, url: %s, status: %d", url, (int)status);
  }

  curl_slist_free_all(headers);
  free(url);

  return status;
}

struct rs_node *get_node_remote(const char *path, bool fetch_body) {

  struct rs_node *node = make_node(path);

  long status = perform_request(path, node, fetch_body);
  if(status > 400) {
    free_node(node);
    return NULL;
  }

  if(node->is_dir && fetch_body) {
    parse_listing(node);
  }

  return node;
}

struct rs_node *get_node_remote_via_parent(const char *path, bool fetch_body) {
  char *name = rs_basename(path);
  char *parent_path = rs_dirname(path);
  struct rs_node *parent_node = get_node_remote(parent_path, true);
  struct rs_node *node = NULL;

  char *dir_name = xmalloc(strlen(name) + 2);
  sprintf(dir_name, "%s/", name);

  char *dir_path = NULL;

  struct rs_dir_entry *entry;
  for(entry = (struct rs_dir_entry*) parent_node->data; entry != NULL; entry = entry->next) {
    if(strcmp(entry->name, name) == 0) {
      node = get_node_remote(path, fetch_body);
      break;
    } else if(strcmp(entry->name, dir_name) == 0) {
      dir_path = xmalloc(strlen(path) + 2);
      sprintf(dir_path, "%s/", path);
      node = get_node_remote(dir_path, fetch_body);
      break;
    }
  }

  free_node(parent_node);
  free(name);
  free(dir_name);
  free(parent_path);

  if(dir_path) {
    free(dir_path);
  }

  return node;
}
