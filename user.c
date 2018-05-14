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
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arg.h"

int x; // cursor's x position
int y; // cursor's y position
int x_win; // number of columns
int y_win; // number of rows
int version; // which version of the file this user has
int prev_len; // the length of previous version's file
int len; // the length of current version
int id; // the id of this user
int real_index; // where this user is at in terms of the real file
char* buf; // current version of the shared doc that this user has
FILE* time_log; // file for recording lag time
size_t send_time; // the time this user sends a change to the server
int total_characters; // for limiting cursor within the file length

// Get the time in microseconds.
// Taken from Virtual Memory Lab. Thanks to Professor Curtsinger.
size_t time_us() {
  struct timeval tv;
  if(gettimeofday(&tv, NULL) == -1) {
    perror("gettimeofday");
    exit(2);
  }

  // Convert timeval values to microseconds
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

// this function finds where the previous newline is in the shared doc
int getnl(int real_index) {
  int limit = real_index - 1;
  // if we are at the first character of the file
  if(limit == -1) {
    return -1;
    // otherwise we traverse to find the previous newline
  } else {
    while (limit >= 0 && buf[limit] != '\n'){
      limit--;
    }
    // if there's no newline before
    if (limit == -1) {
      return -1;
      // if there's a newline, we return the index of this character in the buffer of the file
    } else {
      return limit;
    }
  }
}

// for listening to the server for a newer version of the doc
void* get_new(void* p) {
  // unpack thread arg and then free it
  thread_arg_t* arg = (thread_arg_t*)p;
  int s_for_ds = arg->socket;
  free(arg);
  
  int info[5]; // buffer for version info
  int counter = 0; // how many times we have got a newer version
  
  // keep listening for newer version of the file
  // we first read this version's info
  while(read(s_for_ds, info, sizeof(int) * 5) > 0) {
    version = info[0]; // update this user's version
    prev_len = len; // update the length of the previous version and length of current version
    len = info[1];
    getyx(stdscr, y, x); // save current position of the cursor
    char new_buf[info[1]]; // update the buffer for the file
    buf = new_buf;
    total_characters = info[1] - 2; // update the cursor limiter; notice that we minus 2 instead of 1 because there's usually a newline character appended to the text file
    
    // read the newer version from the server
    if(read(s_for_ds, buf, info[1]) > 0) {
    
      clear(); // clear the terminal
      addstr(buf); // print the newer version on the terminal
      refresh();

      // record time if the change updated in this newer version was made by this user and it is not the initial sending
      if(id == info[2] && counter != 0){
        size_t receive_time = time_us();
        fprintf(time_log, "Time difference: %lu\n", receive_time - send_time);
        fflush(time_log);
      }
      
      // now we update this user's cursor
      // if the change in the newer version is a new line insertion
      if(info[4] == 1 && len-prev_len == 1) {
        // increment the position of the user in the file
        real_index++;
        // if this user didn't make the change and if the change is after the cursor
        if(info[2] != id && info[3] >= real_index) {
          // we don't move
          move(y, x);
          real_index--;
          // if this user didn't make the change and if this change is before the cursor
        } else if (info[2] != id) {
          // we find the previous newline
          int prev_nl = getnl(real_index);
          // we move the cursor to the next line, but stay at where he/she was on this line in the previous version
          move(y + 1, (real_index - prev_nl - 1) % x_win);
          y++;
          x = (real_index - prev_nl - 1) % x_win;
          // if this user made the change, move the cursor to the beginning of the next line
        } else {
          move(y + 1, 0);
          y++;
          x = 0;
        }
        
        // if it is the initial sending: the first time getting the file
      } else if (counter == 0) {
        // we place the cursor at the very first position of the terminal
        move(0, 0);
        x = 0;
        y = 0;
        
        // if the change in the newer version is a new line deletion
      } else if(info[4] == 1 && len-prev_len == -1) {
        // decrement the position of the user in the file
        real_index--;
        // if this user didn't make the change and if the change is after the cursor
        if(info[2] != id && info[3] > real_index + 1) {
          // we don't move the cursor
          move(y, x);
          real_index++;
          // if this user didn't make the change and if the change is before the cursor OR if this user made the change
        } else {
          // we get the position of the previous newline
          int prev_nl = getnl(real_index);
          // we move the cursor to the previous line. If this user didn't make the change, stay at where he/she was on this line in the previous version; otherwise move to the position of deleted newline
          move(y - 1, (real_index - prev_nl - 1) % x_win);
          y--;
          x = (real_index - prev_nl - 1) % x_win;
        }
        
        // if the change in the newer version is an insertion (normal characters)
      } else if (len - prev_len == 1) {
        int prev_nl; // buffer for previous newline's position
        real_index++; // increment the position of the user in the file
        // if this user didn't make the change and if the change is after the cursor
        if(info[2] != id && info[3] >= real_index) {
          // we don't move the cursor
          move(y, x);
          real_index--;
          
          // if this user didn't make the change and if the change is before the cursor and if there's a previous newline
        } else if (info[2] != id && (prev_nl = getnl(real_index)) >= 0) {
          // if this change is on the line where the user is
          if(prev_nl < info[3]) {
            // we move as expected: if not at the end of a line in the terminal, we move to the right; if at the end, we move to the beginning of the next line
            if(x < x_win - 1) {
              move(y, x + 1);
              x++;
            } else {
              move(y + 1, 0);
              y++;
              x = 0;
            }
            // if the change is not on the cursor's line, we don't move
          } else {
            move(y, x);
          }
          
          // if this user inserted OR if this user didn't make the change and if the change is before the cursor and if there's no previous newline, move anyway
        } else {
          if(x < x_win - 1) {
            move(y, x + 1);
            x++;
          } else {
            move(y + 1, 0);
            y++;
            x = 0;
          }
        }
        
        // if the change in the newer version is an deletion (normal characters)
      } else if (len - prev_len == -1) {
        int prev_nl; // buffer for previous newline's position
        real_index--;  // decrement the position of the user in the file
        // if this user didn't make the change and if the change is after the cursor
        if(info[2] != id && info[3] > real_index + 1) {
          // we don't move
          move(y, x);
          real_index++;
          // if this user didn't make the change and if the change is before the cursor and if there's a previous newline 
        } else if(info[2] != id && (prev_nl = getnl(real_index)) >= 0) {
          // if this change is on the line where the user is
          if(prev_nl + 1 < info[3]) {
            // we move as expected: if not at the beginning of a line in the terminal, we move to the left; if at the beginning, we move to the end of the previous line
            if(x > 0) {
              move(y, x - 1);
              x--;
            } else {
              move(y - 1, x_win - 1);
              y--;
              x = x_win - 1;
            }
            // if the change is not on the cursor's line, we don't move
          } else {
            move(y, x);
          }
          
          // if this user deleted OR if this user didn't make the change and if the change is before the cursor and if there's no previous newline, move anyway
        } else {
          if(x > 0) {
            move(y, x - 1);
            x--;
          } else {
            move(y - 1, x_win - 1);
            y--;
            x = x_win - 1;
          }
        }
        
        // default case: if there's no change in this version
      } else {
        // we don't move
        move(y, x);
      }
      refresh();
      // update the iteration number
      counter++;
    }
  }
  // if we reach here, that means the server is down
  fprintf(stderr, "Server is down\n");
  fflush(stderr);
  exit(2);
  return NULL;
}

// this function writes the change made by this user to the server
void server_write(int ch, int location, int version, int socket, FILE* time_log) {
  change_arg_t change_arg;
  change_arg.c = (char) ch;
  change_arg.loc = location;
  change_arg.version = version;
  if(write(socket, &change_arg, sizeof(change_arg_t)) == -1) {
    fprintf(stderr, "Server is down\n");
    fflush(stderr);
    exit(2);
  }
  // record the time when we send the change to the server
  send_time = time_us();
}

int main(int argc, char** argv) {
  // if not correct number of command line args
  if(argc != 3) {
    fprintf(stderr, "Usage: %s <password> <server address>\n", argv[0]);
    fflush(stderr);
    exit(2);
  }
  
  // store the command line args
  char* passwd = argv[1];
  char* server_address = argv[2];
  
  // initialize the globals
  prev_len = 0;
  len = 0;
  real_index = 0;
  buf = NULL;
  
  //initialize screen
  initscr();
  // one char at a time
  cbreak();
  // no echo
  noecho();
  // to get special keys
  keypad(stdscr, TRUE);
  // get COLs and ROWs of the terminal
  getmaxyx(stdscr, y_win, x_win);
  
  // open a socket for connecting to the server
  int s_for_ds = socket(AF_INET, SOCK_STREAM, 0);
  if(s_for_ds == -1) {
    perror("socket failed");
    exit(2);
  }
  struct sockaddr_in addr_for_ds = {
    .sin_family = AF_INET,
    .sin_port = htons(SERVER_PORT)
  };

  // fill in address of the central server
  struct hostent* hp = gethostbyname(server_address);
  bcopy((char *)hp->h_addr, (char *)&addr_for_ds.sin_addr.s_addr, hp->h_length);
  // connect to central server
  if(connect(s_for_ds, (struct sockaddr*)&addr_for_ds, sizeof(struct sockaddr_in))) {
    perror("Connection to central server failed");
    exit(2);
  }

  // after connected, send the password
  char message[PASSWORD_LIMIT];
  strcpy(message, passwd);
  if(write(s_for_ds, message, sizeof(message)) == -1) {
    fprintf(stderr, "Server is down\n");
    fflush(stderr);
    exit(2);
  }

  // read from the central server for this user's id
  if(read(s_for_ds, &id, sizeof(int)) == -1) {
    fprintf(stderr, "Server is down\n");
    fflush(stderr);
    exit(2);
  }

  // open the file for logging time intervals, one file per user
  char log_file[6];
  log_file[0] = id + '0';
  log_file[1] = '.';
  log_file[2] = 't';
  log_file[3] = 'x';
  log_file[4] = 't';
  log_file[5] = '\0';
  if((time_log = fopen(log_file, "w+")) == NULL){
    fprintf(stderr, "Fail to open %s\n", log_file);
    fflush(stderr);
    exit(2);
  }
  
  // launch a thread for listening to the server for updated versions
  thread_arg_t* arg = (thread_arg_t*)malloc(sizeof(thread_arg_t));
  arg->socket = s_for_ds;
  pthread_t thread;
  if(pthread_create(&thread, NULL, get_new, arg)) {
    perror("pthread_create failed");
    exit(2);
  }

  int ch; // buffer for input from the user

  // keep reading from user input
  while(true) {
    refresh();
    ch = getch();
    // check what type the input is
    switch (ch) {
    case KEY_LEFT:
      // if we are at the beginning of the file, we don't move
      if(real_index == 0) {
        move(y, x);
        real_index++;
        // if we are not at the beginning and the previous character is a newline
      } else if (real_index - 1 >= 0 && buf[real_index - 1] == '\n') {
        // if there is only a newline character before the cursor, we move to the beginning position of the terminal
        if(real_index - 2 < 0) {
          move(0, 0);
          y = 0;
          x = 0;
          // if there are more than one characters, we traverse to find the previous of the previous newline
        } else if(real_index - 2 >= 0) {
          int limit = real_index - 2;
          // traverse if we are still in the buffer and if we haven't found one 
          while(limit >= 0 && buf[limit] != '\n'){
            limit--;
          }
          // move to the previous newline character
          move(y-1, (real_index - 2 - limit) % x_win);
          y--;
          x = (real_index - 2 - limit) % x_win;
        }
        // if the previous character is not a newline and we are at the beginning of some line, move to the end of the last line
      } else if(x == 0) {
        move(y - 1, x_win - 1);
        y--;
        x = x_win - 1;
        // if we are in the middle of some line, move to one position left
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
      // if we are within the length of the file
      if (real_index < total_characters){
        // if we are not at the end of some line
        if(x < x_win - 1) {
          // if you are at the newline, move to the next line
          if (buf[real_index] == '\n') {
            move(y + 1, 0);
            y++;
            x = 0;
          } else {
            move(y, x + 1);
            x++;
          }
          // if you are at the end of some line, move to the next line
        } else {
          move(y + 1, 0);
          y++;
          x = 0;
        }
        real_index++;
      }
      break;
      // if the user is quitting
    case '\t':
      close(s_for_ds);
      fclose(time_log);
      endwin();
      exit(0);
      // if the user is deleting a char
    case KEY_BACKSPACE:
    case KEY_DC:
    case 127:
      server_write(DELETE, real_index, version, s_for_ds, time_log);
      break;
      // if the user is inserting a newline
    case NEWLINE:
      server_write(NEWLINE, real_index, version, s_for_ds, time_log);
      break;
      // Default Case: if the user is inserting a normal char
    default:
      server_write(ch, real_index, version, s_for_ds, time_log);
      break;
    }
  }
}


