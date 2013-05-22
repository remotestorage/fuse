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

/* void parse_listing(struct rs_node *node) { */
/*   node->children = xmalloc(sizeof(struct rs_node)); */
/*   memset(node->children, 0, sizeof(struct rs_node)); */
/*   struct rs_node *current = node->children, *last = NULL; */
/*   int i; */
/*   enum { sSTART, sOBJECT, sKEY, sVALUE, sEND } state = sSTART; */
/*   char c; */
/*   char key[1024], value[1024]; */
/*   char *keyptr = key, *valueptr = value; */
/*   for(i=0; i < node->size && state != sEND; i++) { */
/*     c = ((char*)node->data)[i]; */
/*     switch(state) { */
/*     case sSTART: */
/*       if(c == '{') { */
/*         state = sOBJECT; */
/*       } else { */
/*         fprintf(stderr, "Faulty directory listing: %s\n", node->data); */
/*         return; */
/*       } */
/*       break; */
/*     case sOBJECT: */
/*       if(c == '"') { */
/*         state = sKEY; */
/*       } else if(c == ':') { */
/*         state = sVALUE; */
/*       } else if(c == '}') { */
/*         state = sEND; */
/*       } */
/*       break; */
/*     case sKEY: */
/*       if(c == '"') { */
/*         *keyptr = 0; */
/*         state = sOBJECT; */
/*       } else { */
/*         *keyptr++ = c; */
/*       } */
/*       break; */
/*     case sVALUE: */
/*       if(c == ',' || c == '}') { */
/*         *valueptr = 0; */
/*         state = sOBJECT; */
/*         current->name = strdup(key); */
/*         current->rev = strdup(value); */
/*         current->next = xmalloc(sizeof(struct rs_node)); */
/*         last = current; */
/*         current = current->next; */
/*         keyptr = key; */
/*         valueptr = value; */
/*       } else { */
/*         *valueptr++ = c; */
/*       } */
/*     } */
/*   } */
/*   if(last) { */
/*     last->next = NULL; */
/*     free(current); */
/*   } else if(! current->name) { */
/*     node->children = NULL; */
/*     free(current); */
/*   } */
/* } */
