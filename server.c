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

int version; // the version of the file in the server
log_node_t* head; // the head of the list of historical changes
log_node_t* tail; // the tail of the list of historical changes
char password[PASSWORD_LIMIT] = "IloveGrinnell"; // default password for checking if an incoming client should be allowed
sockets_t* users; // the list of users
pthread_mutex_t m; // for locking merging process
pthread_mutex_t m2; // for locking the user list
FILE* file; // the shared doc
char* filename; // the pathname of the shared doc
int size; // size of the list of changes
int num_users; // total number of users, including the users who have quitted the system

// this function removes a user from the user list
void user_remove(int id) {
  pthread_mutex_lock(&m2);
  socket_node_t* cur = users->head;
  socket_node_t* prev = NULL;
  // traverse the user list
  while(cur != NULL) {
    // find this user
    if(cur->id == id) {
      // if this user is the head of the user list
      if(prev == NULL) {
        users->head = cur->next;
        free(cur);
        break;
        // if not
      } else {
        prev->next = cur->next;
        free(cur);
        break;
      }
    }
    // update the pointers, prepare for checking the next node
    cur = cur->next;
    prev = cur;
  }
  pthread_mutex_unlock(&m2);
}

// this function sends the updated file to a client
void send_file(int socket, int id, int real_loc, int newline, FILE* file) {
  // get the length of this updated file
  fseek(file, 0, SEEK_END); // go to the end of the file
  int length = ftell(file); // find the length
  rewind(file); // go back to the beginning of the file

  char* buf = (char*)malloc(sizeof(char)*(length + 1)); // buffer for the whole file, including the null terminator

  // read from the file
  if(fread(buf, length, 1, file) > 0) {
    // check for error
    if(ferror(file)) {
      perror("fread");
      exit(2);
    }
    buf[length] = '\0'; // terminate this buffer
    int info[5]; // buffer for this version's info
    info[0] = version; // the version of this file
    info[1] = length + 1; // the size of the buffer
    info[2] = id; // the id of the user who made this change
    info[3] = real_loc; // the location of this change
    info[4] = newline; // if this change is a newline: 1 if it is, 0 otherwise
    write(socket, info, sizeof(int) * 5); // send the file info
    write(socket, buf, length + 1); // send the file
  }
  free(buf); // free the buffer
  rewind(file); // go back to the beginning of the file
}

// listen for an update to the document from a user
void* thread_fn(void* p) {
  // unpack thread arg
  thread_arg_t* arg = (thread_arg_t*)p;
  int connection_socket = arg->socket; // the socket through which we can talk to this user
  int id = arg->id; // this user's id
  free(arg); // free thread arg struct
  int newline = 0; // if a change is newline
  int type = 0; // type of change: 0 for insertion, 1 for deletion
  send_file(connection_socket, id, 0, newline, file); // send the file to the user for the first time
  change_arg_t change; // buffer for getting the user's change

  // keep listening for updates
  while(read(connection_socket, &change, sizeof(change_arg_t)) > 0) {
    pthread_mutex_lock(&m);
    newline = 0; // reset the newline indicator
    type = 0; // reset the type indicator
    printf("ver is %d, loc is %d\n", change.version, change.loc);
    int real_loc = change.loc; // real location of the change: the location of change in the version of the server
    log_node_t* curr = head; // for traversing the list of historical changes

    // find the first change in the history that we want to compare to, in order to find the real location of this change
    while(curr != NULL) {
      // we want to find the change in the history that has the same base version as this change
      if(curr->ver < change.version) {
        curr = curr->next;
      } else {
        break;
      }
    }

    // we traverse the list and compare the location of this change to the location of a historical change, in order to find real location
    while(curr != NULL) {
      // if the historical change's location is before or at the same location as this change
      if(curr->loc <= real_loc) {
        // if the historical change is an insertion, we increase this change's location
        if(curr->type == 0) {
          real_loc++;
          // if the historical change is a deletion, we decrease this change's location
        } else {
          real_loc--;
        }
      }
      curr = curr->next;
    }

    printf("real_loc is %d\n", real_loc);

    if(real_loc < 0) {
      real_loc = 0;
    }

    // find the length of the file, as described in send_file()
    fseek(file, 0, SEEK_END); 
    int length = ftell(file);
    rewind(file);

    char* dest = (char*)malloc(sizeof(char) * (length + 1)); // buffer for the whole file
    fread(dest, 1, length, file); // read the file into dest
    // check for error
    if(ferror(file)) {
      perror("fread");
      exit(2);
    }
    dest[length] = '\0'; // terminate the buffer
    if(freopen(filename, "w+", file) == NULL) { // reopen the file for rewriting
      fprintf(stderr, "Unable to reopen %s\n", filename);
      fflush(stderr);
      exit(2);
    }
    char* second_part = (char*)malloc(sizeof(char) * (length - real_loc + 1)); // buffer for the part of the file after the change
    
    // if this change is a deletion
    if((int)change.c == DELETE) {
      type = 1; // set the type indicator to 1
      // just to make sure we are not beyond the file
      if(real_loc < length) {
        // write the part of the file before the deletion
        fwrite(dest, real_loc - 1, 1, file);
        // check for error
        if(ferror(file)) {
          perror("fwrite");
          exit(2);
        }
        fflush(file);
        // if this change is to delete a newline, we set the newline indicator to 1
        if(dest[real_loc - 1] == '\n') {
          newline = 1;
        }
        // we copy the part of the file after deletion into second_part
        strcpy(second_part, &dest[real_loc]);
        // terminate the buffer
        second_part[length-real_loc] = '\0';
        // write this part to file
        fwrite(second_part, length-real_loc, 1, file);
        // check for error
        if(ferror(file)) {
          perror("fwrite");
          exit(2);
        }
        fflush(file);
      }
      
      // if this change is an insertion
    } else {
      // just make sure we are not making a change beyond the file
      if(real_loc < length) {
        // write the part of the file before the change
        fwrite(dest, real_loc, 1, file);
        // check for error
        if(ferror(file)) {
          perror("fwrite");
          exit(2);
        }
        fflush(file);
        // add the change
        fputc(change.c, file);
        fflush(file);
        // copy the rest of the file into second_part
        strcpy(second_part, &dest[real_loc]);
        // terminate the buffer
        second_part[length-real_loc] = '\0';
        // write this part to file
        fwrite(second_part, length-real_loc, 1, file);
        // check for error
        if(ferror(file)) {
          perror("fwrite");
          exit(2);
        }
        fflush(file);
      }
      // if this change is a newline, we set the newline indicator to 1
      if((int)change.c == NEWLINE) {
        newline = 1;
      }
    }
    // free the buffers
    free(dest);
    free(second_part);
    // add this change to log history
    log_node_t* new = (log_node_t*)malloc(sizeof(log_node_t));
    new->ver = version; // the version before merging the change
    version++; // update version
    new->loc = real_loc; // the location of the change in the version before the change
    new->type = type; // is this change an insertion or a deletion
    new->next = NULL;
    // add this node to the end of the list of changes
    if(tail != NULL) {
      tail->next = new;
      tail = new;
    } else {
      head = new;
      tail = new;
    }
    // increase the size of the list of changes
    size++;
    // when there are over 100 nodes in the list of changes, we free the head node
    if(size > 100) {
      log_node_t* front = head;
      head = head->next;
      free(front);
    }
    // traverse the user list and send each user the updated file
    socket_node_t* cur = users->head;
    while (cur != NULL) {
      send_file(cur->socket, id, real_loc, newline, file);
      cur = cur->next;
    }
    pthread_mutex_unlock(&m);
  }
  // if read fails, it must be the case that the user quits, so we remove this user from the user list
  user_remove(id);
  return NULL;
}

int main(int argc, char** argv) {
  // check the number of command line arguments
  if(argc != 2) {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    fflush(stderr);
    exit(2);
  }

  // open specified file for reading
  file = fopen(argv[1], "r");
  if(file == NULL) {
    fprintf(stderr, "Unable to open file: %s\n", argv[1]);
    fflush(stderr);
    exit(2);
  }
  // save this file's path in a global so that we are able to reopen it
  filename = argv[1];

  // initialize the list of users
  users = (sockets_t*)malloc(sizeof(sockets_t));
  users->head = NULL;
  
  // initialize log history
  head = NULL;
  tail = NULL;
  
  // initialize locks
  pthread_mutex_init(&m, NULL);
  pthread_mutex_init(&m2, NULL);
  version = 0; // we are initially at Version 0
  size = 0; // no historical changes at first
  num_users = 0; // no users at first
  
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

  // keep looking for new users
  while(true) {
    // Accept a client connection
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(struct sockaddr_in);
    int client_socket = accept(s, (struct sockaddr*)&client_addr, &client_addr_len);
    if(client_socket == -1) {
      perror("accept failed");
      exit(2);
    }
    // buffer for the password entered by our accepted client
    char passwd[PASSWORD_LIMIT];
    // read message from accepted client
    read(client_socket, passwd, sizeof(passwd));
    
    // check if the password match
    if(strcmp(passwd, password) == 0) {
      // add this user to the list of users
      num_users++; // increase the number of users
      socket_node_t* new = (socket_node_t*)malloc(sizeof(socket_node_t));
      new->socket = client_socket;
      new->next = users->head; // add to the beginning of the user list
      new->id = num_users;
      users->head = new;
      // tell the user his/her id
      write(client_socket, &(new->id), sizeof(int));
      // launch a thread for listening to this user for updates to the file
      // first set the thread function argument
      thread_arg_t* arg = (thread_arg_t*)malloc(sizeof(thread_arg_t));
      arg->socket = client_socket;
      arg->id = new->id;
      // launch the thread
      pthread_t thread;
      if(pthread_create(&thread, NULL, thread_fn, arg)) {
        perror("pthread_create failed");
        exit(2);
      }
      // if the password does not match, disconnect to this user
    } else {
      close(client_socket);
    }
  }
}
