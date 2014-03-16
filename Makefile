CC = gcc
CFLAGS = -g -Wall


client: my_client.o swp.o
	$(CC) $(FLAGS) -o client my_client.o swp.o

server: my_server.o swp.o
	$(CC) $(FLAGS) -o server my_server.o swp.o

my_client: client.c swp.h
	$(CC) $(CFLAGS) -c client.c -o my_client

my_server: server.c swp.h
	$(CC) $(CFLAGS) -c server.c -o my_server

swp: swp.c swp.h
	$(CC) $(CFLAGS) -c swp.c -o swp
