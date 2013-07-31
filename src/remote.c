
#include "remotestorage-fuse.h"

CURL *curl_handle;

void init_remote() {
  curl_handle = curl_easy_init();
}

void cleanup_remote() {
  curl_easy_cleanup(curl_handle);
}

struct rs_read_state {
  struct rs_node *node;
  off_t offset;
};

size_t read_body(char *ptr, size_t size, size_t nmemb, void *userdata) {
  struct rs_read_state *state = (struct rs_read_state*) userdata;
  int requested = size * nmemb;
  int remaining = state->node->size - state->offset;
  int actual = remaining < requested ? remaining : requested;
  int i;
  for(i=0;i<actual;i++) {
    ptr[i] = state->node->data[state->offset + i];
  }
  state->offset += actual;
  return actual;
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
  char tmp[bytes + 1];
  memcpy(tmp, ptr, bytes);
  tmp[bytes] = 0;
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


enum rs_method { HEAD, GET, PUT, DELETE };

char *method_names[] = { "HEAD", "GET", "PUT", "DELETE" };

long perform_request(enum rs_method method, const char *path, struct rs_node* node) {
  curl_easy_reset(curl_handle);

  bool fetch_body = method != HEAD;

  curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1);

  // set URL
  char *url = xmalloc(RS_CONFIG.base_url_len + strlen(path) + 1);
  sprintf(url, "%s%s", RS_CONFIG.base_url, path);
  curl_easy_setopt(curl_handle, CURLOPT_URL, url);
  // set headers
  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, RS_CONFIG.auth_header);

  if(method == PUT) {
    headers = curl_slist_append(headers, "Content-Type: application/octet-stream; charset=binary");
  }

  curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);

  log_msg("[remote] %s %s", method_names[method], path);

  struct rs_read_state read_state;

  switch(method) {
  case GET:
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_body);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, node);
    break;
  case HEAD:
    curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 1);

    if(! node->is_dir) {
      curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, write_header);
      curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, node);
    }
    break;
  case PUT:
    read_state.node = node;
    read_state.offset = 0;
    curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, 1);
    curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, read_body);
    curl_easy_setopt(curl_handle, CURLOPT_READDATA, &read_state);
    curl_easy_setopt(curl_handle, CURLOPT_INFILESIZE, node->size);
    log_msg("PUTting %d bytes", node->size);
    break;
  case DELETE:
    log_msg("DELETE NOT IMPLEMENTED!!!");
    return -1;
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

  long status = perform_request(fetch_body ? GET : HEAD, path, node);
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

int put_node_remote(const char *path, struct rs_node *node) {
  long status = perform_request(PUT, path, node);
  return (status == 200 || status == 201) ? 0 : status;
}
