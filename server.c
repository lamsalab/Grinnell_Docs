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
#include <curses.h>
#include "arg.h"

#define DELETE 16
#define NEWLINE 10

typedef struct socket_node {
  int socket;
  int id;
  struct socket_node* next;
} socket_node_t;

// a list of sockets, one for each user
typedef struct sockets {
  socket_node_t* head;
} sockets_t;

int version;
log_node_t* head;
log_node_t* tail;
char password[PASSWORD_LIMIT] = "IloveGrinnell"; // default password
sockets_t* users; // the list of sockets
pthread_mutex_t m; // for locking merging process
FILE* file; // the shared doc
char* filename; // the pathname of the shared doc
int size;
int num_users;

void send_file(int socket, int id, int real_loc, int newline, FILE* file) {
  rewind(file);
  fseek(file, 0, SEEK_END);
  int length = ftell(file);
  rewind(file);
  char buf[length + 1];
  if(fread(buf, length, 1, file) > 0) {
    buf[length] = '\0';
    int* ver_len = (int*)malloc(sizeof(int) * 5);
    ver_len[0] = version;
    ver_len[1] = length + 1;
    ver_len[2] = id;
    ver_len[3] = real_loc;
    ver_len[4] = newline;
    write(socket, ver_len, sizeof(int)*5);
    free(ver_len);
    write(socket, buf, length + 1);
    bzero(buf, length + 1);
  }
  rewind(file);
}

// zero for not a new line
// is it possible to send a blank file?
// listen for updates to the document
void* thread_fn(void* p) {
  // unpack thread arg
  thread_arg_t* arg = (thread_arg_t*)p;
  int connection_socket = arg->socket;
  int id = arg->id;
  free(arg); // free thread arg struct
  int newline = 0;
  send_file(connection_socket, id, 0, newline, file);
  change_arg_t change;

  
  while(read(connection_socket, &change, sizeof(change_arg_t)) > 0) {
    pthread_mutex_lock(&m);
    printf("ver is %d, loc is %d\n", change.version, change.loc);
    int real_loc = change.loc;
    log_node_t* curr = head;
    while(curr != NULL) {
      if(curr->ver < change.version) {
        curr = curr->next;
      } else {
        break;
      }
    }
    while(curr != NULL) {
      if(curr->loc <= real_loc) {
        real_loc++;
      }
      curr = curr->next;
    }
    printf("real_loc is %d\n", real_loc);
    rewind(file);
    fseek(file, 0, SEEK_END);
    int length = ftell(file);
    rewind(file);
    char dest[length + 1];
    fread(dest, 1, length, file);
    dest[length] = '\0';
    freopen(filename, "w+", file);
    if((int)change.c == DELETE) {
      if(real_loc < length) {
        fwrite(dest, real_loc - 1, 1, file);
        fflush(file);
        char second_part[length-real_loc + 1];
        strcpy(second_part, &dest[real_loc]);
        second_part[length-real_loc] = '\0';
        fwrite(second_part, length-real_loc, 1, file);
        fflush(file);
      }
    } else if((int)change.c == NEWLINE) {
        fwrite(dest, real_loc, 1, file);
        fflush(file);
        for(int i = 0; i < 238 - (real_loc % 238); i++) {
          fputc(' ', file);
          fflush(file);
        }
        char second_part[length-real_loc + 1];
        strcpy(second_part, &dest[real_loc]);
        second_part[length-real_loc] = '\0';
        fwrite(second_part, length-real_loc, 1, file);
        fflush(file);
        newline = 1;
    } else {
      if(real_loc < length) {
        fwrite(dest, real_loc, 1, file);
        fflush(file);
        fputc(change.c, file);
        fflush(file);
        char second_part[length-real_loc + 1];
        strcpy(second_part, &dest[real_loc]);
        second_part[length-real_loc] = '\0';
        fwrite(second_part, length-real_loc, 1, file);
        fflush(file);
      } else {
        fwrite(dest, length, 1, file);
        fflush(file);
        fseek(file, real_loc, SEEK_SET);
        fputc(change.c, file);
        fflush(file);
      }
    }
    // add this change to log history
    log_node_t* new = (log_node_t*)malloc(sizeof(log_node_t));
    new->ver = version;
    version++;
    new->loc = real_loc;
    new->next = NULL;
    if(tail != NULL) {
      tail->next = new;
      tail = new;
    } else {
      head = new;
      tail = new;
    }
    size++;
    if(size > 100) {
      log_node_t* fr = head;
      head = head->next;
      free(fr);
    }
    socket_node_t* cur = users->head;
    while (cur != NULL) {
      send_file(cur->socket, id, real_loc, newline, file);
      cur = cur->next;
    }
    pthread_mutex_unlock(&m);
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
  // initialize log history
  head = NULL;
  tail = NULL;
  // initialize lock
  pthread_mutex_init(&m, NULL);
  // we are initially at Version 0
  version = 0;
  size = 0;
  num_users = 0;
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
      num_users++;
      socket_node_t* new = (socket_node_t*)malloc(sizeof(socket_node_t));
      new->socket = client_socket;
      new->next = users->head;
      new->id = num_users;
      users->head = new;
      int* id_copy = (int*)malloc(sizeof(int));
      *id_copy = new->id;
      write(client_socket, id_copy, sizeof(int));
      free(id_copy);
      // launch a thread for listening to this user
      thread_arg_t* arg = (thread_arg_t*)malloc(sizeof(thread_arg_t));
      arg->socket = client_socket;
      arg->id = new->id;
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
