/*
 * Name: Emile Goulard
 *
 * friendlist.c - [Starting code for] a web-based friend-graph manager.
 *
 * Based on:
 *  tiny.c - A simple, iterative HTTP/1.0 Web server that uses the 
 *      GET method to serve static and dynamic content.
 *   Tiny Web server
 *   Dave O'Hallaron
 *   Carnegie Mellon University
 */
#include "csapp.h"
#include "dictionary.h"
#include "more_string.h"

pthread_mutex_t mutex;
dictionary_t *friends;
static void *thread(void* vargp);
static void doit(int fd);
static dictionary_t *read_requesthdrs(rio_t *rp);
static void read_postquery(rio_t *rp, dictionary_t *headers, dictionary_t *d);
static void clienterror(int fd, char *cause, char *errnum, 
                        char *shortmsg, char *longmsg);
static void print_stringdictionary(dictionary_t *d);

static void serve_friends(int fd, dictionary_t *query);
static void serve_befriend(int fd, dictionary_t *query);
static void serve_unfriend(int fd, dictionary_t *query);
static void serve_introduce(int fd, dictionary_t *query);
static int new_request_content(int clientfd, char** content, char* request);
static char* get_friends(char* user);
static void add_friends(char* friend1, char* friend2);
static void remove_friends(char* friend1, char* friend2);

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  pthread_mutex_init(&mutex, NULL);
  friends = make_dictionary(COMPARE_CASE_INSENS, free);
  listenfd = Open_listenfd(argv[1]);

  /* Don't kill the server if there's an error, because
     we want to survive errors due to a client. But we
     do want to report errors. */
  exit_on_error(0);

  /* Also, don't stop on broken connections: */
  Signal(SIGPIPE, SIG_IGN);

  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    if (connfd >= 0) {
      Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                  port, MAXLINE, 0);
      printf("Accepted connection from (%s, %s)\n", hostname, port);

      int* connfdp;
      pthread_t tid;
      connfdp = malloc(sizeof(int));
      *connfdp = connfd;
      pthread_create(&tid, NULL, thread, connfdp);
      pthread_detach(tid);
    }
  }
}

/*
 * doit - handle one HTTP request/response transaction
 */
void doit(int fd) {
  char buf[MAXLINE], *method, *uri, *version;
  rio_t rio;
  dictionary_t *headers, *query;

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);
  if (Rio_readlineb(&rio, buf, MAXLINE) <= 0)
    return;
  printf("%s", buf);
  
  if (!parse_request_line(buf, &method, &uri, &version)) {
    clienterror(fd, method, "400", "Bad Request",
                "Friendlist did not recognize the request");
  } 
  else {
    if (strcasecmp(version, "HTTP/1.0") && strcasecmp(version, "HTTP/1.1")) {
      clienterror(fd, version, "501", "Not Implemented",
                  "Friendlist does not implement that version");
    } 
    else if (strcasecmp(method, "GET") && strcasecmp(method, "POST")) {
      clienterror(fd, method, "501", "Not Implemented",
                  "Friendlist does not implement that method");
    } 
    else {
      headers = read_requesthdrs(&rio);

      /* Parse all query arguments into a dictionary */
      query = make_dictionary(COMPARE_CASE_SENS, free);
      parse_uriquery(uri, query);
      if (!strcasecmp(method, "POST"))
        read_postquery(&rio, headers, query);

      /* For debugging, print the dictionary */
      print_stringdictionary(query);

      if (starts_with("/friends", uri))
      {
        serve_friends(fd, query);
      }
      else if (starts_with("/befriend", uri))
      {
        serve_befriend(fd, query);
      }
      else if (starts_with("/unfriend", uri))
      {
        serve_unfriend(fd, query);
      }
      else if (starts_with("/introduce", uri))
      {
        serve_introduce(fd, query);
      }

      /* Clean up */
      free_dictionary(query);
      free_dictionary(headers);
    }

    /* Clean up status line */
    free(method);
    free(uri);
    free(version);
  }
}

/*
 * read_requesthdrs - read HTTP request headers
 */
dictionary_t *read_requesthdrs(rio_t *rp) {
  char buf[MAXLINE];
  dictionary_t *d = make_dictionary(COMPARE_CASE_INSENS, free);

  Rio_readlineb(rp, buf, MAXLINE);
  printf("%s", buf);
  while(strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    parse_header_line(buf, d);
  }
  
  return d;
}

void read_postquery(rio_t *rp, dictionary_t *headers, dictionary_t *dest) {
  char *len_str, *type, *buffer;
  int len;
  
  len_str = dictionary_get(headers, "Content-Length");
  len = (len_str ? atoi(len_str) : 0);

  type = dictionary_get(headers, "Content-Type");
  
  buffer = malloc(len+1);
  Rio_readnb(rp, buffer, len);
  buffer[len] = 0;

  if (!strcasecmp(type, "application/x-www-form-urlencoded")) {
    parse_query(buffer, dest);
  }

  free(buffer);
}

static char *ok_header(size_t len, const char *content_type) {
  char *len_str, *header;
  
  header = append_strings("HTTP/1.0 200 OK\r\n",
                          "Server: Friendlist Web Server\r\n",
                          "Connection: close\r\n",
                          "Content-length: ", len_str = to_string(len), "\r\n",
                          "Content-type: ", content_type, "\r\n\r\n",
                          NULL);
  free(len_str);

  return header;
}

/*
 * clienterror - returns an error message to the client
 */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) {
  size_t len;
  char *header, *body, *len_str;

  body = append_strings("<html><title>Friendlist Error</title>",
                        "<body bgcolor=""ffffff"">\r\n",
                        errnum, " ", shortmsg,
                        "<p>", longmsg, ": ", cause,
                        "<hr><em>Friendlist Server</em>\r\n",
                        NULL);
  len = strlen(body);

  /* Print the HTTP response */
  header = append_strings("HTTP/1.0 ", errnum, " ", shortmsg, "\r\n",
                          "Content-type: text/html; charset=utf-8\r\n",
                          "Content-length: ", len_str = to_string(len), "\r\n\r\n",
                          NULL);
  free(len_str);
  
  Rio_writen(fd, header, strlen(header));
  Rio_writen(fd, body, len);

  free(header);
  free(body);
}

static void print_stringdictionary(dictionary_t *d) {
  int i, count;

  count = dictionary_count(d);
  for (i = 0; i < count; i++) {
    printf("%s=%s\n",
           dictionary_key(d, i),
           (const char *)dictionary_value(d, i));
  }
  printf("\n");
}

static void *thread(void* vargp)
{
  int connfd = *((int *)vargp);
  free(vargp);
  doit(connfd);
  Close(connfd);
  return NULL;
}

/* 
 * get_friends - retrieves the friends of a user
 */
static char* get_friends(char* user)
{
  pthread_mutex_lock(&mutex);
  dictionary_t* friend_set = dictionary_get(friends, user);
  if(!friend_set)
  {
    pthread_mutex_unlock(&mutex);
    return strdup("");
  }
  int count = dictionary_count(friend_set);
  int friend_index;
  char const** curr_friends = malloc(sizeof(size_t) * (count + 1));
  const char* friend;
  char* friend_list;
  for(friend_index = 0; friend_index < count; friend_index++)
  {
    friend = dictionary_key(friend_set, friend_index);
    curr_friends[friend_index] = friend;
  }
  curr_friends[count] = NULL;
  friend_list = join_strings((const char * const*)curr_friends, '\n');
  free(curr_friends);
  pthread_mutex_unlock(&mutex);
  return friend_list;
}

/* 
 * add_friends - handles the process of friending two users.
 */
static void add_friends(char* friend1, char* friend2)
{
  if(!strcmp(friend1, friend2))
    return;

  pthread_mutex_lock(&mutex);

  if (!dictionary_get(friends, friend1))
    dictionary_set(friends, friend1, make_dictionary(1, NULL));

  dictionary_set(dictionary_get(friends, friend1), friend2, NULL);

  if (!dictionary_get(friends, friend2))
    dictionary_set(friends, friend2, make_dictionary(1, NULL));

  dictionary_set(dictionary_get(friends, friend2), friend1, NULL);
  
  pthread_mutex_unlock(&mutex);
}

/* 
 * remove_friends - handles the process of unfriending two users.
 */
static void remove_friends(char* friend1, char* friend2)
{
  pthread_mutex_lock(&mutex);
  dictionary_t* friends_1 = dictionary_get(friends, friend1);
  dictionary_t* friends_2 = dictionary_get(friends, friend2);
  if (friends_1) 
  {
    dictionary_remove(friends_1, friend2);
    if (dictionary_count(friends_1) == 0)
      dictionary_remove(friends, friend1);
  }
  if (friends_2)
  {
    dictionary_remove(friends_2, friend1);
    if (dictionary_count(friends_2) == 0)
      dictionary_remove(friends, friend2);
  }
  pthread_mutex_unlock(&mutex);
}

/*
 * serve_friends - output all the friends from the user
 */
static void serve_friends(int fd, dictionary_t* query)
{
  char* header;
  char* user = dictionary_get(query, "user");
  char* body;
  body = get_friends(user);
  size_t body_len;
  body_len = strlen(body);

  // Response Headers to Client
  header = ok_header(body_len, "text/html; charset=utf-8");
  Rio_writen(fd, header, strlen(header));
  printf("Response headers:\n");
  printf("%s", header);
  free(header);

  // Response Body to Client
  Rio_writen(fd, body, body_len);
  free(body);
}

 /* 
 * serve_befriend - handles in "friends" as a friend of "user".
 */
static void serve_befriend(int fd, dictionary_t* query)
{
  char* body, *header;
  char* user = dictionary_get(query, "user");
  char** befriends = split_string(dictionary_get(query, "friends"), '\n');
  char* friend = befriends[0];

  // Add and Free Friends
  int friend_index = 0;
  while(friend)
  {
    add_friends(friend, user);
    free(friend);
    friend = befriends[++friend_index];
  }
  free(befriends);
  size_t body_len;
  body = get_friends(user);
  body_len = strlen(body);

  // Response Headers to Client
  header = ok_header(body_len, "text/html; charset=utf-8");
  Rio_writen(fd, header, strlen(header));
  printf("Response headers:\n");
  printf("%s", header);
  free(header);

  // Send Body to Clients
  Rio_writen(fd, body, body_len);
  free(body);
}

 /* 
 * serve_unfriend - handles in "friends" as a friend of "user".
 */
static void serve_unfriend(int fd, dictionary_t* query)
{
  char* body, *header;
  char* user = dictionary_get(query, "user");
  char** unfriends = split_string(dictionary_get(query, "friends"), '\n');
  char* friend = unfriends[0];

  // Remove and Free Friends
  int friend_index = 0;
  while (friend)
  {
    remove_friends(friend, user);
    free(friend);
    friend = unfriends[++friend_index];
  }
  free(unfriends);
  size_t body_len;
  body = get_friends(user);
  body_len = strlen(body);

  // Response Headers to Client
  header = ok_header(body_len, "text/html; charset=utf-8");
  Rio_writen(fd, header, strlen(header));
  printf("Response headers:\n");
  printf("%s", header);
  free(header);

  // Send Body to Clients
  Rio_writen(fd, body, body_len);
  free(body);
}

/*
 * serve_introduce - introduces one user to another
 */
static void serve_introduce(int fd, dictionary_t *query)
{
  char* host = dictionary_get(query, "host");
  char* port = dictionary_get(query, "port");
  char* user = dictionary_get(query, "user");
  char* friend = query_encode(dictionary_get(query, "friend"));
  char* header;

  if (host == NULL) 
  {
    clienterror(fd, "GET", "400", "Bad Request", "Invalid Host");
  }
  if (friend == NULL) 
  {
    clienterror(fd, "GET", "400", "Bad Request", "Invalid Friend");
  }
  if (port == NULL) 
  {
    clienterror(fd, "GET", "400", "Bad Request", "Invalid Port");
  }
  if (user == NULL) 
  {
    clienterror(fd, "GET", "400", "Bad Request", "Invalid Username");
  }

  int clientfd = Open_clientfd(host, port);
  char* request = append_strings("GET /friends?user=", friend, " HTTP/1.1\r\n\r\n", NULL);
  char* request_content;
  size_t content_length = new_request_content(clientfd, &request_content, request);
  add_friends(user, friend);
  if(content_length)
  {
      char** friends_list = split_string(request_content, '\n');
      char* friend = friends_list[0];
      int i = 0;
      int byte_buffer = 0;
      while (byte_buffer < content_length)
      {
        byte_buffer += strlen(friend) + 1;
        add_friends(friend, user);
        free(friend);
        friend = friends_list[++i];
      }
      free(request_content);
      free(friends_list);
  }

  // Response Headers to Client
  header = ok_header(0, "text/html; charset=utf-8");
  Rio_writen(fd, header, strlen(header));
  free(request);
  free(header);
  free(friend);
  return;
}

static int new_request_content(int clientfd, char** content, char* request)
{
  int size = strlen(request);
  size_t content_length = 0;
  char buf[MAXLINE], *status;
  Rio_writen(clientfd, request, size);

  rio_t rio;
  Rio_readinitb(&rio, clientfd);
  dictionary_t* headers = make_dictionary(COMPARE_CASE_INSENS, free);

  Rio_readlineb(&rio, buf, MAXLINE);
  parse_status_line(buf, NULL, &status, NULL);
  while (strcmp(buf, "\r\n"))
  {
    Rio_readlineb(&rio, buf, MAXLINE);
    printf("%s", buf);
    parse_header_line(buf, headers);
  }
  printf("Header Dictionary: \n");
  print_stringdictionary(headers);

  if (strcmp(status, "200"))
  {
    printf("Server responded with status %s", status);
  }
  else
  {
    content_length = atoi(dictionary_get(headers, "Content-length"));
    if (content_length)
    {
      *content = malloc(content_length);
      Rio_readnb(&rio, *content, content_length);
    }
  }
  free(status);
  free_dictionary(headers);
  return content_length;
}
