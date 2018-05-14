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

#define PASSWORD_LIMIT 20 // upper limit for number of chars in a password
#define SERVER_PORT 1057 // the port used by the central server
#define DELETE 16
#define NEWLINE 10

// this is the struct that a user will send to the server if there's a change to the document
typedef struct change_arg {
  char c; // the change
  int loc; // the location of the change
  int version; // the version the user is in when the user makes the change
  int id; // this user's id
} change_arg_t;

// this struct represents thread function argument
typedef struct thread_arg {
  int socket; // the socket through which we can talk to somebody
  int id; // if this 'somebody' is a client, this represents id of that client
} thread_arg_t;

// this struct represents a node in the log of historical changes
typedef struct log_node {
  int ver; // the change is merged based on
  int loc; // location of this change when it is merged
  int type; // 0 for insertion, 1 for deletion
  struct log_node* next;
} log_node_t;

// this struct represents a user in the eyes' of the server
typedef struct socket_node {
  int socket; // socket through which we can talk to this user
  int id; // this user's id
  struct socket_node* next;
} socket_node_t;

// this struct represents a list of users
typedef struct sockets {
  socket_node_t* head;
} sockets_t;


