CC=gcc
CFLAGS=-Wall
LDLIBS=-pthread

all: server client

server: server.c
	$(CC) $(CFLAGS) $(LDLIBS) -o server server.c

client: client.c
	$(CC) $(CFLAGS) $(LDLIBS) -o client client.c

server_debug: server.c
	gcc -g $(CFLAGS) $(LDLIBS) -o server.db server.c

.PHONY: clean
clean:
	rm server client