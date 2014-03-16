CC = gcc
CFLAGS = -g -Wall

all: swp client server

client: swp.o client.o swp.h
	$(CC) $(CFLAGS) -o client client.o swp.o

server: swp.o server.o swp.h
	$(CC) $(CFLAGS) -o server server.o swp.o

my_client: client.c swp.h
	$(CC) $(CFLAGS) -c client.c -o client

my_server: server.c swp.h
	$(CC) $(CFLAGS) -c server.c -o server

swp: swp.c swp.h
	$(CC) $(CFLAGS) -c swp.c -o swp
