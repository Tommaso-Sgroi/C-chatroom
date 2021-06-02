CC=gcc
CFLAGS=-Wall
LDLIBS=-pthread

all: server client

server: server.c
	$(CC) $(CFLAGS) $(LDLIBS) -o server.o server.c

client: client.c
	$(CC) $(CFLAGS) $(LDLIBS) -o client.o client.c

clean:
	rm -f *.o server client
