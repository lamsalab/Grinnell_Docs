CC=clang
CFLAGS=-g

all: central_server user

clean:
	rm -f central_server user

central_server:central_server.c
	$(CC) $(CFLAGS) -o central_server central_server.c  -lncurses -lpthread

user: user.c
	$(CC) $(CFLAGS) -o user user.c -lncurses

