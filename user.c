#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <curses.h>
#include "arg.h"

int x; // cursor's x position
int y; // cursor's y position
int x_win; // number of columns
int y_win; // number of rows
int version;

int total_characters;

// for listening to the server for a newer version of the doc
void* get_new(void* p) {
  // unpack thread arg
  thread_arg_t* arg = (thread_arg_t*)p;
  int s_for_ds = arg->socket;
  free(arg); // free thread arg struct
  int info[2];
  // Read lines until we hit the end of the input (the client disconnects)
  while(read(s_for_ds, info, sizeof(int) * 2) > 0) {
    version = info[0];
    getyx(stdscr, y, x);
    char buf[info[1]];
    total_characters = info[1] - 1;
    if(read(s_for_ds, buf, info[1]) > 0) {
      clear();
      addstr(buf);
      refresh();
      move(y, x);
      refresh();
    }
  } 
  return NULL;
}

int main(int argc, char** argv) {
  // if not correct number of command line args
  if(argc != 3) {
    fprintf(stderr, "Usage: %s <password> <server address>\n", argv[0]);
    fflush(stderr);
    exit(EXIT_FAILURE);
  }
  
  // store the command line args
  char* passwd = argv[1];
  char* server_address = argv[2];

  // after connected, initialize screen and send message to join the system
  initscr();
  // one char at a time
  cbreak();
  // no echo
  noecho();
  // to get special keys
  keypad(stdscr, TRUE);
  // get COLs and ROWs
  getmaxyx(stdscr, y_win, x_win);
  // connect to directory server
  int s_for_ds = socket(AF_INET, SOCK_STREAM, 0);
  if(s_for_ds == -1) {
    perror("socket failed");
    exit(2);
  }
  struct sockaddr_in addr_for_ds = {
    .sin_family = AF_INET,
    .sin_port = htons(SERVER_PORT)
  };

  // fill in address of directory server
  struct hostent* hp = gethostbyname(server_address);
  bcopy((char *)hp->h_addr, (char *)&addr_for_ds.sin_addr.s_addr, hp->h_length);
  
  if(connect(s_for_ds, (struct sockaddr*)&addr_for_ds, sizeof(struct sockaddr_in))) {
    perror("I failed: connect failed");
    exit(2);
  }

  // after connected, send the password
  user_arg_t* message = (user_arg_t*)malloc(sizeof(user_arg_t));
  strcpy(message->buffer, passwd);
  write(s_for_ds, message, sizeof(user_arg_t));

  // launch a thread for listening to the server
  thread_arg_t* arg = (thread_arg_t*)malloc(sizeof(thread_arg_t));
  arg->socket = s_for_ds;
  pthread_t thread;
  if(pthread_create(&thread, NULL, get_new, arg)) {
    perror("pthread_create failed");
    exit(EXIT_FAILURE);
  }
  refresh();
  int ch;
  change_arg_t* change_arg = (change_arg_t*)malloc(sizeof(change_arg_t));
  // move cursor according to user input
  while(true) {
    refresh();
    ch = getch();
    switch (ch) {
    case KEY_LEFT:
      if(x > 0) {
        move(y, x - 1);
        x--;
      }
      break;
    case KEY_UP:
      if(y > 0) {
        move(y-1, x);
        y--;
      }
      break;
    case KEY_DOWN:
      if ((y+1)*x_win + x < total_characters){
        move(y+1, x);
        y++;
      }
      break;
    case KEY_RIGHT:
      if(x < x_win - 1) {
        if (y*x_win + (x+1) < total_characters){
          move(y, x+1);
          x++;
        }
      }
      break;
    case KEY_BACKSPACE:
    case KEY_DC:
    case 127:
      change_arg->c = (char)10000;
      change_arg->loc = y*x_win + x;
      change_arg->version = version;
      write(s_for_ds, change_arg, sizeof(change_arg_t));
      if(x > 0) {
        move(y, x-1);
        x--;
      } else if (x == 0) {
        if(y > 0) {
          move(y-1, x_win-1);
          y--;
          x = x_win - 1;
        }
      }      
      break;
      // if the input is not for moving a cursor, but for inserting a char
    default:
      change_arg->c = ch;
      change_arg->loc = y*x_win + x;
      change_arg->version = version;
      write(s_for_ds, change_arg, sizeof(change_arg_t));
      move(y, x+1); // to do: check if need a boundary
      x++;
      break;
    }
  }
  endwin();
}
