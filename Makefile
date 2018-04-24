CC = clang
CFLAGS = -g

all: test_curses

clean:
	rm -f test_curses

test_curses: test_curses.c
	$(CC) $(CFLAGS) -o test_curses test_curses.c -lncurses
