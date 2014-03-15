CC = gcc
CFLAGS = -g -Wall

all: client server
client: client.c message.h
	$(CC) $(CFLAGS) -o client client.c
server: server.c message.h
	$(CC) $(CFLAGS) -o server server.c
