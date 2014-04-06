CC = gcc
CFLAGS = -g -Wall

all: mim client server

client: client.c message.h
	$(CC) $(CFLAGS) -o client client.c

server: server.c message.h
	$(CC) $(CFLAGS) server.c -o server

mim: mim.c message.h
	$(CC) $(CFLAGS) mim.c -o mim

clean: 
	rm client server mim
