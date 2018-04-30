#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "arg.h"

typedef struct socket_node {
  int socket;
  struct socket_node* next;
} socket_node_t;

// a list of sockets, one for each user
typedef struct sockets {
  socket_node_t* head;
} sockets_t;

typedef struct change_node {
  char c;
  int loc;
  struct change_node* next;
} change_node_t;

typedef struct changes {
  change_node_t* head;
} changes_t;

char password[PASSWORD_LIMIT] = "IloveGrinnell"; // default password
sockets_t* users; // the list of sockets
pthread_mutex_t m; // for locking the list
FILE* file; // the shared doc
char* filename; // the pathname of the shared doc
changes_t* changes;

void send_file(int socket, FILE* file) {
  char buf[255];
  rewind(file);
  while(fgets(buf, 255, file) != NULL) {
    write(socket, buf, 255);
    bzero(buf, 255);
  }
  rewind(file);
}

// listen for updates to the document
void* thread_fn(void* p) {
  // unpack thread arg
  thread_arg_t* arg = (thread_arg_t*)p;
  int connection_socket = arg->socket;
  free(arg); // free thread arg struct
  send_file(connection_socket, file);
  change_arg_t change;
  while(read(connection_socket, &change, sizeof(change_arg_t)) > 0) {
    rewind(file);
    fseek(file, 0, SEEK_END);
    int length = ftell(file);
    rewind(file);
    char dest[length + 1];
    fread(dest, 1, length, file);
    dest[length] = '\0';
    freopen(filename, "w+", file);
    if(change.loc <= length) {
    fwrite(dest, change.loc, 1, file);
    fflush(file);
     fputc(change.c, file);
     fflush(file); 
     char second_part[length-change.loc + 1];
     strcpy(second_part, &dest[change.loc]);
     second_part[length-change.loc] = '\0';
     fwrite(second_part, length-change.loc, 1, file);
     fflush(file);
    } else {
      fwrite(dest, length, 1, file);
      fflush(file);
      fputc(change.c, file);
      fflush(file);
    }
     socket_node_t* cur = users->head;
     while (cur != NULL) {
     send_file(cur->socket, file);
     cur = cur->next;
     }
  }
    return NULL;
}

int main(int argc, char** argv) {
  
  if(argc != 2) {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    fflush(stderr);
    exit(EXIT_FAILURE);
  }

  // open specified file
  file = fopen(argv[1], "r");
  if(file == NULL) {
    fprintf(stderr, "Unable to open file: %s\n", argv[1]);
    fflush(stderr);
    exit(EXIT_FAILURE);
  }
  filename = argv[1];

  // initialize a list of users
  users = (sockets_t*)malloc(sizeof(sockets_t));
  users->head = NULL;
  // initialize history of changes
  changes = (changes_t*)malloc(sizeof(changes_t));
  changes->head = NULL;
  pthread_mutex_init(&m, NULL);
  
  // set up a socket for accepting new clients
  int s = socket(AF_INET, SOCK_STREAM, 0);
  if(s == -1) {
    perror("socket failed");
    exit(2);
  }

  // the sockaddr_in struct that will be bound to our socket in order to accept new clients
  struct sockaddr_in addr = {
    .sin_addr.s_addr = INADDR_ANY,
    .sin_family = AF_INET,
    .sin_port = htons(SERVER_PORT)
  };
    
  // Bind our socket to our sockaddr_in struct
  if(bind(s, (struct sockaddr*)&addr, sizeof(struct sockaddr_in))) {
    perror("bind");
    exit(2);
  }

  // Begin listening on SERVER_PORT for new clients
  if(listen(s, 2)) {
    perror("listen failed");
    exit(2);
  }

  // accepting new clients and handling their requests
  while(true) {
    // Accept a client connection
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(struct sockaddr_in);
    int client_socket = accept(s, (struct sockaddr*)&client_addr, &client_addr_len);
    if(client_socket == -1) {
      perror("accept failed");
      exit(2);
    }
    // buffer for args from our accepted client
    user_arg_t arg;
    // read message from accepted client
    int bytes_read = read(client_socket, &arg, sizeof(user_arg_t));
    if(bytes_read < 0) {
      perror("read failed");
      exit(2);
    }
    // check if the password match
    if(strcmp(arg.buffer, password) == 0) {
      // add the socket to the list of sockets
      socket_node_t* new = (socket_node_t*)malloc(sizeof(socket_node_t));
      new->socket = client_socket;
      pthread_mutex_lock(&m);
      new->next = users->head;
      users->head = new;
      pthread_mutex_unlock(&m);
      // launch a thread for listening to this user
      thread_arg_t* arg = (thread_arg_t*)malloc(sizeof(thread_arg_t));
      arg->socket = client_socket;
      pthread_t thread;
      if(pthread_create(&thread, NULL, thread_fn, arg)) {
        perror("pthread_create failed");
        exit(EXIT_FAILURE);
      }
      // if the password does not match
    } else {
      write(client_socket, "Not Allowed\n", 12);
      close(client_socket);
    }
  }
}
