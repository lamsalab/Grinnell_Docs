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
#define MSG_LIMIT 256
#define MAX_USERS 100
FILE* target_file;
char* pswd = "abcd";

pthread_mutex_t lock;


typedef struct threadarg{
  int socket;

}threadarg_t;


void send_file(int client_socket){

  //send file
      
  char size[MSG_LIMIT];
        FILE*temp=target_file;
        fseek (temp, 0, SEEK_END);
        int length = ftell (temp);
        fseek (temp, 0, SEEK_SET);
        char* dest;
        dest=(char*)malloc(sizeof(char)*length);
        fread (dest, 1, length, temp);
        //SEND SIZE OF FILE
        sprintf(size,"%d",length);
        if(send(client_socket, size,MSG_LIMIT,0) == -1){
          printf("message: %s\n", strerror(errno));
          exit(2);
        }

        //SEND FILE
        if(send(client_socket, dest,MSG_LIMIT,0) == -1){
          printf("message: %s\n", strerror(errno));
          exit(2);
        }
}

void* listening_thread_fn(void* p){
  char c;
threadarg_t* arg= (threadarg_t*)p;
  int client_socket = arg->socket;
   while(true){
     //receive message
     if(read(client_socket, &c,1)==-1){
       printf("message: %s \n", strerror(errno));
       exit(1);
     }

     pthread_mutex_lock(&lock);
     
     if(fputc(c,target_file)==EOF){
       printf("Failed to write to local file\n");
       exit(2);
     }
     
     send_file(client_socket);
     
     pthread_mutex_unlock(&lock);



 }
}


int main(){

  pthread_mutex_init(&lock,NULL);

  //initialize a  counter for users
  int users=0;


  pthread_t listening_threads[MAX_USERS];

  // Open local file to send
  target_file = fopen("local_copy.txt", "rw");




  // Set up a socket
  int s = socket(AF_INET, SOCK_STREAM, 0);
  if(s == -1) {
    perror("socket");
    exit(2);
  }

  // Listen at this address. We'll bind to port 4444 to accept at this port
  struct sockaddr_in addr = {
    .sin_addr.s_addr = INADDR_ANY,
    .sin_family = AF_INET,
    .sin_port = htons(4445)
  };

  // Bind to the specified address
  if(bind(s, (struct sockaddr*)&addr, sizeof(struct sockaddr_in))) {
    perror("bind");
    exit(2);
  }

  // Become a server socket by listening
  if(listen(s, 2)){
   perror("listen failed");
    exit(2);
    }

  // Get the listening socket info so we can find out which port we're using
  socklen_t addr_size = sizeof(struct sockaddr_in);
  getsockname(s, (struct sockaddr *) &addr, &addr_size);

  int listening_addr= ntohs(addr.sin_port);

  // Print the port information
  printf("Listening on port %d\n", ntohs(addr.sin_port));


  //being accepting connections from client
 int client_socket;
  socklen_t client_addr_len = sizeof(struct sockaddr_in);

  //waiting to accept connection from any new identity:
  while(true){
    if(users<MAX_USERS){
      client_socket = accept(s, (struct sockaddr*)&addr, &client_addr_len);
      if(client_socket==-1){
        perror("Accept Connection failed");
        exit(2);
      }
      //up till here connection is established

      char password[MSG_LIMIT];

      // Read password from client
      if(read(client_socket, password, MSG_LIMIT)==-1){
        printf("message: %s \n", strerror(errno));
        exit(1);
      }


      if(!(strcmp(password, pswd)==0)){
        if(write(client_socket, "You do not have access to this file \n", MSG_LIMIT-1)==-1){
          printf("message: %s \n", strerror(errno));
          exit(1);
        }
      }
      else{
        //send file
        send_file(client_socket);
       
       
        threadarg_t args;
        args.socket=client_socket;
        //setup listening thread
        if(pthread_create(&listening_threads[users],NULL,listening_thread_fn,&args)!=0){
          perror("Failed to create listening thread \n");
          exit(1);
        }
      }
      users++;
    }
  }
}
