CFLAGS=-ansi -pedantic-errors -Wall -g -std=c99 -D_GNU_SOURCE

all: file_client file_server

file_client: file_client.o client_protocol.o network.o
	gcc $(CFLAGS) file_client.o client_protocol.o network.o -o file_client

client_protocol.o: client_protocol.c client_protocol.h network.h
	gcc $(CFLAGS) -c client_protocol.c
	
file_client.o: file_client.c client_protocol.h
	gcc $(CFLAGS) -g -c file_client.c
	
network.o: network.c network.h
	gcc $(CFLAGS) -g -c network.c
	
file_server: server_protocol.o file_server.o network.o
	gcc $(CFLAGS) -g server_protocol.o file_server.o network.o -o file_server
	
server_protocol.o: server_protocol.c server_protocol.h network.h
	gcc $(CFLAGS) -c server_protocol.c
	
file_server.o: file_server.c server_protocol.h
	gcc $(CFLAGS) -c file_server.c

clean:
	rm -f *.o file_client file_server
