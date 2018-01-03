/*
 * client_protocol.h
 *
 *  Created on: Nov 10, 2017
 *      Author: mayacahana
 */

#ifndef CLIENT_PROTOCOL_H_
#define CLIENT_PROTOCOL_H_
#include "network.h"
#include "server_protocol.h" // remove this after bridge fixed

#include <stdio.h>

void chopN(char* str, size_t n);
int defineUser(int serverSocket);
void createMessageCommand(Message* m,  MessageType type, char* prefix);
int createQuitCommand(Message* m, int mySocketfd);
int sendClientCommand(char* commandStr, int mySocketfd);
int addFileClientSide(char* buffer, char* filePath);
int getFileClientSide(char* filePath, char* fileBuffer);
int  client_start(char* hostname, int port);
int listOfFilesCommand(Message* m, char* commandStr,  int mySocketfd);
int deleteFileCommand(Message* m, char* commandStr, int mySocket);
int addFileCommand(Message* m, char* path_to_file,char* file_name, int mySocket);
int getFileCommand(Message* m, char* file_name, char* path_to_save, int mySocket);




#endif /* CLIENT_PROTOCOL_H_ */
