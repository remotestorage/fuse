#include "remotestorage-fuse.h"

#include <stdarg.h>

FILE *logfile;

void open_log() {
  logfile = fopen("rs-fuse.log", "a+");

  if(! logfile) {
    perror("Failed to open log");
    abort();
  }

  //logfile = stderr; 
}

void log_msg(char *format, ...) {
  va_list ap;
  va_start(ap, format);
  vfprintf(logfile, format, ap);
  va_end(ap);
  fprintf(logfile, "\n");
  fflush(logfile);
}


char *adjust_path(const char *path, bool is_dir) {
  char *clean_path = path;
  int len = strlen(path);
  if(len == 1) {
    return path;
  }
  if(is_dir && path[len - 1] != '/') {
    clean_path = xmalloc(len + 2);
    sprintf(clean_path, "%s/", path );
  }// I think we should test for files with '/' in the end and beeing files
  return clean_path;
}

char *strip_slash(const char *path) {
  int len = strlen(path);
  char *ret = xmalloc(len);
  strncpy(ret, path, len - 1);
  return ret;
}

char* get_parrent_path(const char* path){
  char* tmp = strdup(path);
  char* ret = adjust_path(dirname(tmp), true);
  free(tmp);
  return ret;
}

bool is_dir(const char* path){
  return (path[strlen(path)-1]=='/');
}


char *rs_dirname(const char *path) {
  int last_slash = 0, i;
  for(i=0;path[i+1] != 0; i++) {
    if(path[i] == '/') {
      last_slash = i;
    }
  }
  int dname_len = last_slash + 1;
  char *dname = xmalloc(dname_len + 1);
  strncpy(dname, path, dname_len);
  return dname;
}

char *rs_basename(const char *path) {
  int last_slash = 0, i;
  for(i=0;path[i+1] != 0; i++) {
    if(path[i] == '/') {
      last_slash = i;
    }
  }
  if(last_slash == 0 && i == 0) { // handle root path
    last_slash--;
  }
  int bname_len = i - last_slash;
  char *bname = xmalloc(bname_len + 1);
  strncpy(bname, path + last_slash + 1, bname_len);
  return bname;
}

#ifdef DEBUG_HELPERS

void assert(char *a, char *b) {
  if(strcmp(a, b) != 0) {
    fprintf(stderr, "Assertion failed: %s != %s\n", a, b);
    abort();
  }
}

int main(int argc, char **argv) {

  if(is_dir("/") != true) {
    fprintf(stderr, "is_dir broken!\n");
    abort();
  }

  assert(rs_basename("/"), "/");
  assert(rs_dirname("/"), "/");
  assert(rs_basename("/foo/bar"), "bar");
  assert(rs_basename("/foo/bar/"), "bar/");
  assert(rs_dirname("/foo/bar"), "/foo/");
  assert(rs_dirname("/foo/bar/"), "/foo/");

}

#endif

void parse_listing(struct rs_node *node) {
  char *listing = (char*)node->data;
  node->data = xmalloc(sizeof(struct rs_dir_entry));
  struct rs_dir_entry *current = (struct rs_dir_entry*)node->data, *last = NULL;
  int i;
  enum { sSTART, sOBJECT, sKEY, sVALUE, sEND } state = sSTART;
  char c;
  char key[1024], value[1024];
  char *keyptr = key, *valueptr = value;
  for(i=0; i < node->size && state != sEND; i++) {
    c = listing[i];
    switch(state) {
    case sSTART:
      if(c == '{') {
        state = sOBJECT;
      } else {
        fprintf(stderr, "Faulty directory listing: %s\n", listing);
        return;
      }
      break;
    case sOBJECT:
      if(c == '"') {
        state = sKEY;
      } else if(c == ':') {
        state = sVALUE;
      } else if(c == '}') {
        state = sEND;
      }
      break;
    case sKEY:
      if(c == '"') {
        *keyptr = 0;
        state = sOBJECT;
      } else {
        *keyptr++ = c;
      }
      break;
    case sVALUE:
      if(c == ',' || c == '}') {
        *valueptr = 0;
        state = sOBJECT;
        current->name = strdup(key);
        current->rev = strdup(value);
        current->is_dir = is_dir(current->name);
        current->next = xmalloc(sizeof(struct rs_dir_entry));
        last = current;
        current = current->next;
        keyptr = key;
        valueptr = value;
      } else {
        *valueptr++ = c;
      }
    }
  }
  if(last) {
    last->next = NULL;
    free(current);
  } else if(! current->name) {
    node->data = NULL;
    free(current);
  }
}
