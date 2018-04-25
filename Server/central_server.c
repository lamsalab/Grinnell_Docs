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
#define MAX_USERS 100

typedef struct target{
  FILE *target_file;
  pthread_mutex_t lock;}

off_t fsize(const char *filename) {
    struct stat st; 

    if (stat(filename, &st) == 0)
        return st.st_size;

    return -1; 
}

typedef struct threadarg{
  int socket;

}theadarg_t;

typedef struct filecontent{
  char* content;

}filecontent_t;

void* listening_thread_fn(void* p){
  char c;
thread_arg_t* arg= (thread_arg_t*)p;
  int client_socket = arg->socket; 
   char buffer[MSG_LIMIT];
   while(true){
     //receive message
     if(read(client_socket, &c,1)==-1){
       printf("message: %s \n", strerror(errno));
       exit(1);
     }
  
 pthread
    


}


int main(){
  list_t list;
  list_init(list);

  pthread_mutex_init(&lock);

  //initialize a  counter for users
  int users=0;
  
  // Open users file with all permitted usernames
  FILE *file;
  file=fopen("permitted_users.txt", "r");

  pthread_t listening_threads[MAX_USERS];
  
  // Open local file to sen 
  target_file = fopen("local_copy.txt", "rw");
  
  char* buffer=malloc(sizeof(char)*MSG_LIMIT);
  unsigned short cur;
  
  while( fgets(buffer,MSG_LIMIT,file)!=NULL){
    cur=atoi(buffer);
    list_add(list,cur);
  }

  fclose(file);

  
  
  // Set up a socket
  int s = socket(AF_INET, SOCK_STREAM, 0);
  if(s == -1) {
    perror("socket");
    exit(2);
  }

  // Listen at this address. We'll bind to port 4444 to accept any available port
  struct sockaddr_in addr = {
    .sin_addr.s_addr = INADDR_ANY,
    .sin_family = AF_INET,
    .sin_port = htons(4444)
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
    if(client<MAX_USERS){
      client_socket = accept(s, (struct sockaddr*)&addr, &client_addr_len);
      if(client_socket==-1){
        perror("Accept Connection failed");
        exit(2);
      }
      //up till here connection is established

      //read initial information from client
      if(read(client_socket, &buffer, MSG_LIMIT-1)==-1){
        printf("message: %s \n", strerror(errno));
        exit(1);
      }

      cur=atoi(buffer);

      if(!has_username(list,cur)){
        if(write(client_socket, "You do not have access to this file \n", MSG_LIMIT-1)==-1){
          printf("message: %s \n", strerror(errno));
          exit(1);
        }
      }
      else{
        //send file
        off_t size=fsize(target_file);
        if(size==-1){
          printf("Unable to read file");
          exit(1);
        }
        filecontent_t dest;
        dest->content=(char*)malloc(sizeof(char)*size);
        FILE*temp=target_file;
        fseek (temp, 0, SEEK_END);
        length = ftell (temp);
        fseek (f, 0, SEEK_SET);
        fread (dest->content, 1, length, f);
        if(send(client_socket, dest,size,0) == -1){
          printf("message: %s\n", strerror(errno));
          exit(2);
        }
        threadarg_t args;
        args.socket=client_socket;
        //setup listening thread
        if(pthread_create(&listening_thread[client],NULL,listening_thread_fn,&args)!=0){
          perror("Failed to create listening thread \n");
          exit(1);
        }
      }
    }
  }
}
