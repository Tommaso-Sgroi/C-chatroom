CC=gcc
CFLAGS=-Wall
LDLIBS=-pthread

all: server client

server: server.c
	$(CC) $(CFLAGS) $(LDLIBS) -o server server.c

client: client.c
	$(CC) $(CFLAGS) $(LDLIBS) -o client client.c

clean:
	rm server
	rm client
