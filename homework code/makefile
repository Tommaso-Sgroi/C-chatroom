SOURCE1=server.c
OUT=server.o
CC=gcc
CFLAGS=-Wall
LDLIBS=-pthread

server: server.c
	$(CC) $(CFLAGS) server.c $(LDLIBS)


client: client.c
	gcc -pthread client.c -lpthead
