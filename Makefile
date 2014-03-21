CC = gcc
CFLAGS = -g -Wall

all: mim client server

client: swp.o my_client.o
	$(CC) $(CFLAGS) -o client my_client.o swp.o

server: swp.o my_server.o
	$(CC) $(CFLAGS) -o server my_server.o swp.o

mim: my_mim.o swp.o
	$(CC) $(CFLAGS) -o mim my_mim.o swp.o

my_client.o: client.c swp.o
	$(CC) $(CFLAGS) -o my_client.o -c client.c

my_server.o: server.c swp.o
	$(CC) $(CFLAGS) -c server.c -o my_server.o

my_mim.o: mim.c swp.o
	$(CC) $(CFLAGS) -c mim.c -o my_mim.o

swp.o: swp.c swp.h
	$(CC) $(CFLAGS) -c swp.c -o swp.o

clean: 
	rm client server my_server.o my_client.o swp.o

