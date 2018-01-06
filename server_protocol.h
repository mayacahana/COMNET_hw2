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
	int online;
} User;

Message* createServerMessage(MessageType type, char* arg1);
void freeUsers(int numOfUsers);
void addFile(int clientSocket, Message* msg, User* user);
void deleteFile(int clientSocket, Message* msg, User* user);
void sendListOfFiles(int clientSocket, User* user);
void sendFileToClient(int clientSocket, Message* msg, User* user) ;
int handleMessage(int clientSocket, Message* msg, User* user);
char* getNameAndFiles(User* user);
int client_serving(int clientSocket, int numOfUsers);
void sendGreetingMessage(int clientSocket);
void start_listen(int numOfUsers, int port);
void start_server(char* users_file, const char* dir_path, int port);
void sendListOfOnlineUsers(int clientSocket, User* user);
void readMessages(int clientSocket, User* user);
void messageOtherUser(int clientSocket,Message* msg, User* user);
User* getUser(char* username);
int login(int clientSocket);
int build_fd_sets(fd_set *read_fds, fd_set* write_fds, fd_set *except_fds);


#endif /* SERVER_PROTOCOL_H_ */
