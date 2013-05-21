
#include "remotestorage-fuse.h"

#include <assert.h>

//typedef enum { false = 0, true = 1 } bool;

#define BPT_ORDER 4

typedef struct node Node;

struct node {
  int is_leaf :1;
  int is_data :1;
  int padding :6;
  Node** data;//[BPT_ORDER];
  unsigned int length;
  Node* neighbor;
  Node* parent;
  char* key;
  char* value;
};

Node* root_node;

Node* new_node(char *key, Node* parent){
  assert(key != NULL);
  fprintf(stderr, "creating node with key = %s\n", key);
  Node* node = xmalloc(sizeof(node));
  node->data = xmalloc(BPT_ORDER * sizeof(Node*));
  node->parent = parent;
  node->key = strdup(key);
  return node;
}

void free_node(Node *node) {
  free(node->data);
  free(node);
}

Node* _search(char* key,Node* node){
  fprintf(stderr, "searching %s in %s\n",key, node->key);
  if(node->is_leaf){
    return node;
  }
  Node** data = node->data;
  unsigned int len = node->length;
  for(unsigned int i = 0; i < len; i++){
    if(strcmp(key, data[i]->key) <= 0){
      return _search(key, data[i]);
    }
  }
  return _search(key, data[len-1]);
}

Node* search(char* key){
  return _search(key, root_node);
}

Node* _insert(char* key, char* value, Node* node);

Node* split(Node* node, Node** new_data, unsigned int count){
  fprintf(stderr, "splitting %s, count: %d", node->key, count);

  Node* right_node = new_node(new_data[count-1]->key, NULL);
  node->neighbor = right_node;
  memcpy(new_data, right_node->data, count * sizeof(Node*));
  if(node->parent->length+1 < BPT_ORDER){
    node->parent->data[node->parent->length] = right_node;
    right_node->parent = node->parent;
    return NULL;
  } else {
    if(! node->parent) {
      node->parent = new_node(right_node->key, NULL);
    }
    return _insert(right_node->key, NULL, node->parent ); // this is wrong
  }
}

Node* _insert(char* key, char* value, Node* node){
  assert(key != NULL);
  assert(value != NULL);
  assert(node != NULL);

  unsigned int len = node->length;
  Node** data = node->data;
  Node** buf = xmalloc((len + 1) * sizeof(Node*));
  
  Node* data_node = new_node(key, node);
  data_node->is_data = true;
  data_node->value = strdup(value);
  
 
  unsigned int i = 0;
  for( ; i < len; i++){
    int res = strcmp(key,data[i]->key);
    if(res == 0){
      fprintf(stderr, "setting value of 0x%x to %s\n", data[i], data_node->value);
      data[i]->value = data_node->value; //node with the same key found
      free_node(data_node);
      free(buf);
      return NULL;
    }else if(res > 0){
      buf[i] = data_node;
      fprintf(stderr, "copying remaining %d nodes\n", (len - i));
      memcpy(buf + (i+1), data + i,(len-i) * sizeof(Node*));
      break;
    }else{
      fprintf(stderr, "setting buf[%d] to data[%d] (i.e. 0x%x)\n", i, i, data[i]);
      buf[i]=data[i];
    }
  }

  if(++len < BPT_ORDER){  // new data still fit into the node
    if(buf[0] == NULL) { // first insert.
      buf[0] = data_node;
    }
    fprintf(stderr, "attempt to copy %d node pointers into node->data. buf[0] is 0x%x, node->data is 0x%x\n", len, buf[0], node->data);
    memcpy(node->data, buf, len * sizeof(Node*));
    if(i < len-1) { //if data not inserted at the end then 
      fprintf(stderr, "setting key to %s\n", key);
      node->key = key;  // node->key now holds the largest key in the node
    } else {
      fprintf(stderr,"not setting node->key to %s, i is %d, len-1 is %d\n",
              key, i, len-1);
    }
    node->length = len;
    free(buf);
    return NULL;
  } else { // node needs spliting
    len = BPT_ORDER/2; //at least half filled
    memcpy(node->data, buf, len * sizeof(Node*));
    node->length = len;
    node->key = strdup(node->data[len-1]->key);
    Node* ret =  split(node, buf + len, BPT_ORDER-BPT_ORDER/2); //looks ugly but renders to a constant
    free(buf);
    return ret;
  }

}

void insert(char *key,char *value){
  fprintf(stderr, "inserting %s => %s\n", key, value);
  Node* node = search(key);
  if( node = _insert(key, value, node) ){
    root_node = node;
  }
  return;
}

void inspect(Node *node) {
  fprintf(stderr, "NODE 0x%x", node);
  fprintf(stderr, " key: %s,", node->key);
  fprintf(stderr, " is_leaf: %s,", node->is_leaf ? "true" : "false");
  fprintf(stderr, " is_data: %s,", node->is_data ? "true" : "false");
  fprintf(stderr, " length: %d,", node->length);
  fprintf(stderr, " value: %s\n", node->value);
}

void dump_tree(Node *node, int depth) {
  assert(node != NULL);
  for(int i=0;i<depth;i++) {
    fprintf(stderr, "  ");
  }
  inspect(node);
  depth++;
  for(int i=0;i<node->length;i++) {
    dump_tree(node->data[i], depth);
  }
}

int main(int argc, char **argv) {
  printf("starting now\n");
  root_node = new_node("/", NULL);
  root_node->is_leaf = true;
  dump_tree(root_node, 0);
  insert("/foo/bar", "baz");
  dump_tree(root_node, 0);
  insert("/foo/blubb", "bla");
  dump_tree(root_node, 0);
  insert("/asdf/dassf/fdas", "blubb");
  dump_tree(root_node, 0);
  insert("/asdd/f/dsa/s", "fasd");
  dump_tree(root_node, 0);
}
