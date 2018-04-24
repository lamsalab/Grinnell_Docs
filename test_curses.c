#include <curses.h>
#include <stdio.h>
#include <stdlib.h>

  int x_position_cursor;
  int y_position_cursor;

int main () {

  char * str = "Hello, I am a test string";

  //The very first thing to do
  WINDOW * main_screen = initscr();
  if(main_screen == NULL) {
    fprintf(stderr, "Failed to initialize screen\n");
    exit(EXIT_FAILURE);
  }
  
  //Get input one character at a time
  cbreak();

  //Suppress automatic printing
  noecho();

  //Handles arrow keys
  keypad(main_screen, TRUE);

  x_position_cursor = 0;
  y_position_cursor = 0;

  //Default the cursor to 0, 0
  wmove(main_screen, 0, 0);

  //Refresh the screen
  wrefresh(main_screen);

  //Add the string to the screen
  //waddstr(main_screen, str);

  int x = 0;
  int y = 0;

  //getmaxyx(main_screen, x, y);
  //waddch(main_screen, x);
  


  int ch;
  while(true) {
    ch = getch();
    switch (ch) {
    case KEY_LEFT:
      wmove(main_screen, y_position_cursor, x_position_cursor-1);
      x_position_cursor--;      
      break;
    case KEY_UP:
      wmove(main_screen, y_position_cursor-1, x_position_cursor);
      y_position_cursor--;
      break;
    case KEY_DOWN:
      wmove(main_screen, y_position_cursor+1, x_position_cursor);
      y_position_cursor++;
      break;
    case KEY_RIGHT:
      wmove(main_screen, y_position_cursor, x_position_cursor+1);
      x_position_cursor++;
      break;
    case KEY_BACKSPACE:
      wmove(main_screen, y_position_cursor, x_position_cursor-1);
      x_position_cursor--;
      break;
   default:
     waddch(main_screen, ch);
     wrefresh(main_screen);
     wmove(main_screen, y_position_cursor, x_position_cursor+1);
     x_position_cursor++;
     break;
      }
  }
  
  //Exit
  endwin();
  
}
