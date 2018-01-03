/*
 * server_protocol.h
 *
 *  Created on: Nov 10, 2017
 *      Author: mayacahana
 */

#ifndef SERVER_PROTOCOL_H_
#define SERVER_PROTOCOL_H_
#include "network.h"


typedef struct user_t {
	char* user_name;
	char* password;
	char* dir_path;
} User;

Message* createServerMessage(MessageType type, char* arg1);
void freeUsers(User** usersArray, int numOfUsers);
void addFile(int clientSocket, Message* msg, User* user);
void deleteFile(int clientSocket, Message* msg, User* user);
void sendListOfFiles(int clientSocket, User* user);
void sendFileToClient(int clientSocket, Message* msg, User* user) ;
int handleMessage(int clientSocket, Message* msg, User* user);
char* getNameAndFiles(User* user);
int client_serving(int clientSocket, User** users, int numOfUsers);
void sendGreetingMessage(int clientSocket);
void start_listen(User** usersArray, int numOfUsers, int port);
void start_server(char* users_file, const char* dir_path, int port);



#endif /* SERVER_PROTOCOL_H_ */
