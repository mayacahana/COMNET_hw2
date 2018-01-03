/*
 * network.h
 *
 *  Created on: Nov 14, 2017
 *      Author: mayacahana
 */

#ifndef NETWORK_H_
#define NETWORK_H_

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>


#define MAX_USERNAME_SIZE 25
#define MAX_PASSWORD_SIZE 25
#define MAX_CLIENTS 15
#define MAX_FILES_PER_CLIENT 15
#define MAX_FILE_SIZE 512
#define MAX_PATH_NAME 500
#define MAX_ARG_LEN 500
#define MAX_FILE_NAME 50
#define MAX_COMMAND_NAME 15
#define HEADER_SIZE (sizeof(MessageHeader))
#define MAX_PACKET_SIZE 4096 // ask
#define MAX_DATA_SIZE (MAX_PACKET_SIZE - HEADER_SIZE)


typedef enum messageType{
	LOGIN_DETAILS,
	LIST_OF_FILES,
	DELETE_FILE,
	ADD_FILE,
	GET_FILE,
	QUIT,
	ERROR,
	INVALID_LINE,
	GREETING,
	FILE_CONTENT
} MessageType;

typedef struct Header_t{
	short type;
	short arg1len;
} MessageHeader;

typedef struct Message_t{
	MessageHeader header;
	char arg1[MAX_PACKET_SIZE];
} Message;

void printMessageArg(Message* msg);

int receiveAll(int socket, void* buffer, int* len);

int send_command(int sckt, Message* msg_to_sent);

int receive_command(int sckt, Message* msg_recieved);

#endif
