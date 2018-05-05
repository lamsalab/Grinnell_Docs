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

#define PASSWORD_LIMIT 20
#define SERVER_PORT 1035

// this is the struct that a user will send to the server if there's a change to the document
typedef struct change_arg {
  char c; // the change
  int loc; // the location of the change
  int version;
} change_arg_t;

// this is the struct that a user will send to the server if the user wants to join
typedef struct user_arg {
  char buffer[PASSWORD_LIMIT];
} user_arg_t;

// this struct represents thread function argument
typedef struct thread_arg {
  int socket; // the socket through which we can talk to somebody
} thread_arg_t;

// this struct represents a node in the log of history
typedef struct log_node {
  int ver; // based on
  int loc; // loc when version is ver
  struct log_node* next;
} log_node_t;

