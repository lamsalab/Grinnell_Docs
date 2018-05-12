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
#include <stdbool.h>
#include "arg.h"

int x; // cursor's x position
int y; // cursor's y position
int x_win; // number of columns
int y_win; // number of rows
int version;
int prev_len;
int len;
int id;
int real_index;
char* buf;

int total_characters;

int getnl(int real_index) {
  int limit = real_index - 1;
  if(limit == -1) {
    return -1;
  } else {
    while (limit >=0 && buf[limit] != '\n'){
      limit--;
    }
    if (limit == -1) {
      return -1;
    } else {
      return limit;
    }
  }
}

// for listening to the server for a newer version of the doc
void* get_new(void* p) {
  // unpack thread arg
  thread_arg_t* arg = (thread_arg_t*)p;
  int s_for_ds = arg->socket;
  free(arg); // free thread arg struct
  int info[5];
  int counter = 0;  
  // Read lines until we hit the end of the input (the client disconnects)
  while(read(s_for_ds, info, sizeof(int) * 5) > 0) {
    version = info[0];
    prev_len = len;
    len = info[1];
    getyx(stdscr, y, x);
    buf=(char*) realloc(buf, sizeof(char)*info[1]);
    total_characters = info[1] - 2;
    if(read(s_for_ds, buf, info[1]) > 0) {
      clear();
      addstr(buf);
      refresh();
      // if a new line (insertion)
      if(info[4] == 1 && len-prev_len == 1) {
        real_index++;
        if(info[2] != id && info[3] >= real_index) {
          move(y, x);
          real_index--;
        } else if (info[2] != id) {
          int prev_nl = getnl(real_index);
          move(y+1, (real_index - prev_nl - 1)% x_win);
          y++;
          x = (real_index - prev_nl - 1)% x_win;
        } else {
          move(y+1, 0);
          y++;
          x = 0;
        }
        // if the first version
      } else if (counter == 0) {
        move(0, 0);
        x = 0;
        y = 0;
        // if a newline (deletion)
      } else if(info[4] == 1 && len-prev_len == -1) {
        real_index--;
        // if after, we don't change
        if(info[2] != id && info[3] > real_index + 1) {
          move(y, x);
          real_index++;
          // if before
        } else {
          int prev_nl = getnl(real_index);
          move(y-1, (real_index - prev_nl - 1) % x_win);
          y--;
          x = (real_index - prev_nl - 1) % x_win;
        } 
        // if insertion (normal char)
      } else if (len-prev_len == 1) {
        int prev_nl;
        real_index++;
        if(info[2] != id && info[3] >= real_index) {
          move(y, x);
          real_index--;
          // if before
        } else if (info[2] != id && (prev_nl = getnl(real_index)) >= 0) {
          if(prev_nl < info[3]) {
            if(x < x_win -1) {
          move(y, x+1);
          x++;
          } else {
            move(y+1, 0);
            y++;
            x = 0;
          }
          } else {
            move(y, x);
          }
          // if this client inserted this char, move anyway
        } else {
          if(x < x_win -1) {
          move(y, x+1);
          x++;
          } else {
            move(y+1, 0);
            y++;
            x = 0;
          }
        }
        // if deletion
      } else if (len-prev_len == -1) {
        int prev_nl;
        real_index--;
        if(info[2] != id && info[3] > real_index + 1) {
          move(y, x);
          real_index++;
        } else if(info[2] != id && (prev_nl = getnl(real_index)) >= 0) {
          if(prev_nl + 1 < info[3]) {
            if(x > 0) {
            move(y, x-1);
            x--;
          } else {
            move(y-1, x_win -1);
            y--;
            x = x_win -1;
            }
          } else {
            move(y, x);
          }
          // if this client deleted, move anyway
        } else {
          if(x > 0) {
            move(y, x-1);
            x--;
          } else {
            move(y-1, x_win -1);
            y--;
            x = x_win -1;
          }
        }
        // default
      } else {
        move(y, x);
      }
      refresh();
    }
    counter++;
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

  prev_len = 0;
  len = 0;
  real_index = 0;
  buf = NULL;
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
  free(message);
  read(s_for_ds, &id, sizeof(int));
  // launch a thread for listening to the server
  thread_arg_t* arg = (thread_arg_t*)malloc(sizeof(thread_arg_t));
  arg->socket = s_for_ds;
  pthread_t thread;
  if(pthread_create(&thread, NULL, get_new, arg)) {
    perror("pthread_create failed");
    exit(EXIT_FAILURE);
  }
  //refresh();
  int ch;
  change_arg_t* change_arg = (change_arg_t*)malloc(sizeof(change_arg_t));
  // move cursor according to user input
  while(true) {
    refresh();
    ch = getch();
    switch (ch) {
    case KEY_LEFT:
      if(real_index == 0) {
        move(y, x);
        real_index++;
      } else if (real_index-1>= 0 && buf[real_index - 1] == '\n') {
        if(real_index -2 < 0) {
          move(0, 0);
          y=0;
          x=0;
        } else if(real_index - 2 >= 0) {
          int limit = real_index - 2;
          while ( limit>=0 && buf[limit] != '\n'){
            limit--;
          }
          int dist;
          if (limit ==-1){
            dist=(real_index-1)%x_win;
          } else{
            dist=(real_index-2-limit) %x_win;
          }
          move(y-1, dist);
          y--;
          x=dist;
        }
      } else if(x == 0) {
          move(y-1, x_win-1);
          y--;
          x = x_win-1;
        } else {
        move(y, x - 1);
        x--;
      }
      real_index--;
      break;
    case KEY_DOWN:
    case KEY_UP:
      move(y, x);
      break;
    case KEY_RIGHT:
      if (real_index < total_characters){
         if(x < x_win - 1) {
          if (buf[real_index] == '\n') {
            move(y+1, 0);
            y++;
            x = 0;
          } else {
            move(y, x+1);
            x++;
          }
         } else {
           move(y+1, 0);
           y++;
         }
         real_index++;
      }
      break;
    case KEY_BACKSPACE:
    case KEY_DC:
    case 127:
      change_arg->c = (char)16;
      change_arg->loc = real_index;
      change_arg->version = version;
      write(s_for_ds, change_arg, sizeof(change_arg_t));
      break;
    case 10:
      change_arg->c = (char)10;
      change_arg->loc = real_index;
      change_arg->version = version;
      write(s_for_ds, change_arg, sizeof(change_arg_t));
       break;
      // if the input is not for moving a cursor, but for inserting a char
    default:
      change_arg->c = ch;
      change_arg->loc = real_index;
      change_arg->version = version;
      write(s_for_ds, change_arg, sizeof(change_arg_t));
      break;
    }
  }
  endwin();
}
