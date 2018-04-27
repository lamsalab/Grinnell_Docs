#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include "linked_list.c"
#define MSG_LIMIT 256

int connect_to_server(int to_socket){
    struct sockaddr_in addr2 = {
      .sin_addr.s_addr = INADDR_ANY,
      .sin_family = AF_INET,
      .sin_port = htons(0)
    };

    if(connect(to_socket, (struct sockaddr*) &addr2, sizeof(struct sockaddr_in)) == -1) {
      //connection fail
      printf("connection failed \n");
      return 0;
    }

    // Connection success
    return 1;
}

char* read_file(int socket) {
  char buffer[MSG_LIMIT];

  // Read filesize over the socket
  if(read(socket, buffer, MSG_LIMIT) == -1) {
    printf("message: %s \n", strerror(errno));
    exit(1);
  }
  int filesize=atoi(buffer);

  // Allocate memory for file to be copied into
  char* file = (char*)malloc(sizeof(filesize));

  // Read file
  if(read(socket, file, initial_size) == -1) {
    printf("message: %s \n", strerror(errno));
    exit(1);
  }

  return file;
}

int main(int argc, char** argv) {
  if(argc != 3) {
    fprintf(stderr, "Usage: %s <server address> <password>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  char* password = argv[2];
  // Connect to server
  char* server_address = argv[1];

  int directory_socket = socket(AF_INET, SOCK_STREAM, 0);
  if(directory_socket == -1){
    perror("server socket failed");
    exit(2);
  }
    // Connect to directory server
  if(connect_to_server(htons(atoi(server_address)), directory_socket) == 0) {
      perror("connect failed");
      exit(2);
    }
  // Figure out our ip address or port number

  if(write(directory_socket, password, strlen(password)) == -1) {
    printf("message: %s \n", strerror(errno));
    exit(1);
  }

   // We should have connected so read initial file and display it

   char* file= read_file(directory_socket);

   // Display file
   free(file);

  // Infinitely receive copy of document while sending new version
  int running=1;

  while(running){
    if(fgets(buffer,MSG_LIMIT,stdin)==NULL){
      perror("Unable to read input");
      exit(1);
    }

    if(strcmp(buffer,"exit")==0){ break;}

      int length=strlen(buffer);
      for(int i=0;i<length;i++){
        if(write(directory_socket, buffer[i], 1) == -1) {
          printf("message: %s \n", strerror(errno));
          exit(1);
        }

        char* file = read_file(directory_socket);
        // Display file again on UI

        free(file);
  }
 }
}
