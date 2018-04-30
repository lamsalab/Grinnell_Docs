CC = clang
CFLAGS = -g

all: user server

clean:
	rm -f user server

user: user.c 
	$(CC) $(CFLAGS) -o user user.c -lncurses -pthread

server: server.c 
	$(CC) $(CFLAGS) -o server server.c -lpthread
