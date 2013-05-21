
void *xmalloc(size_t size) {
  void *ptr = malloc(size);
  if(! ptr) {
    perror("malloc failed");
    abort();
  }
  return ptr;
}

// response filled by send_request
struct rs_response {
  int parse_state;
  int status;
  char *body;

  // header
  int content_length;
  char *transfer_encoding;
};

// metadata of a file / directory
struct rs_meta {
  int is_dir;
  off_t bytelength;
};

void parse_line(char *line, struct rs_response *response) {
  switch(response->parse_state) {
  case 0: // status code
    response->status = atoi(line);
    response->parse_state++;
    break;
  case 1: // header
    char *value_ptr = strtok(line, ":");
    if(value_ptr) {
      line[value_ptr - line - 1] = 0;
      while(*value_ptr == ' ') {
        value_ptr++;
      }
      if(strcmp("Content-Length", line) == 0) {
        response->content_length = atoll(value_ptr);
      } else if(strcmp("Transfer-Encoding", line) == 0) {
        response->transfer_encoding = strdup(value_ptr);
      }
    } else if(*line != ' ' && *line != '\t') {
      response->parse_state++;
    }
  }
}

void send_request(const char* verb, const char* path, struct rs_response *response)
{
  struct addrinfo addr_hints;
  struct addrinfo *addr_result;
  memset(&addr_hints, 0, sizeof(struct addrinfo));
  addr_hints.ai_family = AF_INET;
  addr_hints.ai_socktype = SOCK_STREAM;
  int gai_error = getaddrinfo(RS_HOST, "http", &addr_hints, &addr_result);
  if(gai_error != 0) {
    fprintf(stderr, "getaddrinfo() failed: %s\n", gai_strerror(gai_error));
    abort();
  }
  int sockfd = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);
  if(socket < 0) {
    perror("socket() failed");
    abort();
  }
  struct addrinfo *rp;
  int connected = 0;
  for(rp = addr_result; rp != NULL; rp = rp->ai_next) {
    if(connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) {
      connected = 1;
      break;
    }
  }
  if(! connected) {
    perror("connect() failed");
    abort();
  }
  send(sockfd, verb, strlen(verb), 0);
  send(sockfd, " ", 1, 0);
  send(sockfd, path, strlen(path), 0);
  send(sockfd, " HTTP/1.1\r\n", 11, 0);
  send(sockfd, "Host: " RS_HOST "\r\n", strlen(RS_HOST) + 8, 0);
  send(sockfd, "Connection: close\r\n\r\n", 21, 0);
 
  memset(response, 0, sizeof(struct rs_response));

  size_t received;
  char line_buf[1025] = "", *lnp = line_buf;
  char buf[1025];
  while((received = recv(sockfd, buf, 1024, 0)) > 0) {
    buf[received] = 0;
    int i;
    if(response->parse_state < 2 ) {
      for(i=0;i<received;i++) {
        *lnp++ = buf[i];
        if(buf[i] == '\n' || (lnp - line_buf) > 1024) {
          *lnp = 0;
          parse_line(line_buf, response);
          *line_buf = 0;
          lnp = line_buf;
        }
      }
    } else {
      if(response->content_length) {
        response->body = xmalloc(response->content_length);
        recv(sockfd, response->body, response->content_length, 0);
      } else if(response->transfer_encoding) {
        if(strncmp("chunked", response->transfer_encoding, 7) == 0) {
          recv(sockfd, buf, 1024,0);
          long long int chunk_size = strtoll(buf,&buf,16);
          // TODO the sun is rising, where to start reading whats up with the rest lets rest.

          response->body = realloc(response->body, response->content_length+=chunk_size)
          recv(sockfd,response->body, chunk_size, 0);

        }
      }
    }
  }
}

const char *get_header(struct rs_response *response, const char *name) {
  return "0";
}

struct rs_meta *get_metadata_cache(const char *path) {
  return NULL;
}

void put_metadata_cache(const char *path, struct rs_meta *data) {
}

struct rs_meta *get_metadata_remote(const char *path) {
  struct rs_response response;
  struct rs_meta *data=NULL;
  send_request("HEAD", path, &response);
  if(response.status == 200) {
    data = xmalloc(sizeof(struct rs_meta));
    data->is_dir = path[strlen(path)-1] == '/' ? 0 : 1;
    data->bytelength = response->content_length;
  } else {
    //FIXME maybe wa want to follow a redirect later, maybe in send??
  }
  put_metadata_cache(path,data);
  return data;
}

struct rs_meta *get_metadata(const char *path) {
  struct rs_meta *data = get_metadata_cache(path);
  if(! data) {
    data = get_metadata_remote(path);
  }
  return data;
}
