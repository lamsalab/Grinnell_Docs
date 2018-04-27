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

int main(int argc, char** argv) {
  if(argc != 3) {
    fprintf(stderr, "Usage: %s <username> <server address>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

    int s = socket(AF_INET, SOCK_STREAM, 0);
  if(s == -1) {
    perror("socket");
    exit(2);
  }

   // Listen at this address. We'll bind to port 0 to accept any available port
  struct sockaddr_in addr = {
    .sin_addr.s_addr = INADDR_ANY,
    .sin_family = AF_INET,
    .sin_port = htons(0)
  };

  // Bind to the specified address
  if(bind(s, (struct sockaddr*)&addr, sizeof(struct sockaddr_in))) {
    perror("bind");
    exit(2);
  }

  //connect to server
  char* local_user = argv[1];
  char* server_address = argv[2];

  int directory_socket = socket(AF_INET, SOCK_STREAM, 0);
  if(directory_socket == -1){
    perror("server socket failed");
    exit(2);
  }
    //connect to directory server
  if(connect_to_server(htons(atoi(server_address)), directory_socket) == 0) {
      perror("connect failed");
      exit(2);
    }



  //infintely receive copy of document while sending new version
  int running=1;
  char buffer[MSG_LIMIT];
  while(running){
    fgets(buffer,MSG_LIMIT,stdin);

    if(strcmp(buffer,"exit")==0){ break;}
  while(true){








  }
 }
}
