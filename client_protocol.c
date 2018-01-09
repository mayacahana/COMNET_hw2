#include "client_protocol.h"



/*
 * @param pointer to string and size_t variable n
 * removes last n bits from the string.
 */
void chopN(char* str, size_t n) {
	if (!str) {
		return;
	}
	size_t len = strlen(str);
	if (n > len)
		return;
	memmove(str, str + n, len - n + 1);
}

int delete_connection(connection_t * con) {
	if (close(con->socket) < 0) {
		perror("ERROR:");
		return -1;
	}
	return 0;
}

int create_connection(int socket, char* username) {
	connection_t* newConnection = (connection_t*) malloc(sizeof(connection_t));
	if (!newConnection) {
		return -1;
	}
	newConnection->socket = socket;
	newConnection->username = username;
	return 0;
}

/*
 * sends server username and password
 * returns 0 if succeeds, otherwise returns 1
 */
int defineUser(int scket) {
	int status = 1;
	Message* user_msg = (Message*) malloc(sizeof(Message));
	user_msg->header.type = LOGIN_DETAILS;
	if (!user_msg) {
		printf("malloc failed\n");
		fflush(NULL);
		status = 0;
	}

	int userMaxLen = MAX_USERNAME_SIZE + strlen("User: ");
	int passMaxLen = MAX_PASSWORD_SIZE + strlen("Password: ");
	int loginMaxLen = MAX_USERNAME_SIZE + strlen("User: ") + MAX_PASSWORD_SIZE
			+ strlen("Password: ");

	char* fullUsername = (char*) malloc(sizeof(char) * (userMaxLen));
	char* loginDetails = (char*) malloc(sizeof(char) * (loginMaxLen));
	char* fullPassword = (char*) malloc(sizeof(char) * (passMaxLen));
	loginDetails[loginMaxLen] = '\0';
	char passwordPrefix[11];
	char userPrefix[7];
	userPrefix[6] = '\0';
	passwordPrefix[10] = '\0';
	fullUsername[strlen(fullUsername) - 1] = '\0';
	while (status && user_msg->header.type != QUIT) {
		fgets(fullUsername, userMaxLen, stdin);
		fgets(fullPassword, passMaxLen, stdin);
		strncpy(passwordPrefix, fullPassword, 10);
		strncpy(userPrefix, fullUsername, 6);
		chopN(fullUsername, strlen("User: "));
		chopN(fullPassword, strlen("Password: "));
		if (strcmp(fullUsername, "quit\n") != 0
				&& strcmp(fullPassword, "quit\n") != 0) {
			int passFlag = strcmp(passwordPrefix, "Password: ");
			int userFlag = strcmp(userPrefix, "User: ");
			if (passFlag||userFlag) {
				printf("Wrong prefix of 'User:' or 'Password:' \n");
				free(fullPassword);
				free(fullUsername);
			} else {
				//handle login
				strcpy(loginDetails, fullUsername);
				strcat(loginDetails, "\n");
				strcat(loginDetails, fullPassword);
				createMessageCommand(user_msg, LOGIN_DETAILS, loginDetails);
				status = send_command(scket, user_msg);
				if (status) {
					printf("Error in sending user details\n");
					return status;
				}
				else
				{
					status = receive_command(scket, user_msg);
					if (user_msg->header.type == INVALID_LINE) {
						printf("inavlid username or password\n");
						status = 1;
					} else {
						status = 0;
						printMessageArg(user_msg);
					}
				}
				
			}
		} else {
			//handle quit
			status = createQuitCommand(user_msg, scket);
			if (status) {
				printf("Error in creating quit message\n");
			}
			status = send_command(scket, user_msg);
			if (status) {
				printf("Error in quit\n");
			}
		}
	}
	free(fullPassword);
	free(fullUsername);
	free(loginDetails);
	free(user_msg);
	return status;
}

/*
 * @param Message* m, MessageType type, char* string
 * receives the message type and string, and assigns them to the
 * relevant field in the message struct.
 */
void createMessageCommand(Message* m, MessageType type, char* string) {
	if (m == NULL)
		return;
	m->header.type = type;
	if (string == NULL) {
		strcpy(m->arg1, "");
		return;
	}
	strcpy(m->arg1, string);
	m->header.arg1len = strlen(string) + 1;
	return;
}

/*
 * @param Message *m
 * creates Message struct via createMessageCommand;
 */
int createQuitCommand(Message* m, int mySocketfd) {
	createMessageCommand(m, QUIT, "00");
	int status = send_command(mySocketfd, m);
	if (status != 0) {
		printf("Error. re-send message %s\n", strerror(errno));
		return 1;
	}
	return 1;
}
/*
 * @param Message* m, char* commandStr, int mySocketfd
 * creates Message struct vie createMessageCommand;
 * sends the Message we created to server. If succeeds reveives message from server
 * if send did not succeed, print message to user and exits.
 */
int listOfFilesCommand(Message* m, char* commandStr, int mySocketfd) {
	createMessageCommand(m, LIST_OF_FILES, "list");
	int status = send_command(mySocketfd, m);
	if (status != 0) {
		printf("error, re-send message\n");
		free(m);
		return 0;
	}
	free(m);
	return status;
}

int listOfOnlineUsersCommand(Message*m, char* commandStr, int mySocketfd) {
	createMessageCommand(m, LIST_OF_ONLINE_USERS, "online");
	int status = send_command(mySocketfd, m);
	if (status != 0) {
		printf("error, re-send message\n");
		free(m);
		return 0;
	}
	free(m);
	return status;
}

int deleteFileCommand(Message* m, char* file_name, int mySocket) {
	createMessageCommand(m, DELETE_FILE, file_name);
	int status = send_command(mySocket, m);
	if (status != 0) {
		printf("error, re-send message\n");
		free(m);
		return 1;
	}
	status = receive_command(mySocket, m);
	if (!status) {
		printf("error in receiving message\n");
	}
	printMessageArg(m);
	free(m);
	return 0;
}

int readMessagesClient(Message* m, int mySocket) {
	createMessageCommand(m, READ_MSGS, "read_msgs");
	int status = send_command(mySocket, m);
	if (status != 0) {
		printf("error, re-send message\n");
		free(m);
		return 1;
	}
	printf("\n");
	free(m);
	return 0;
}

int addFileCommand(Message* m, char* path_to_file, char* file_name,
		int mySocket) {
	createMessageCommand(m, ADD_FILE, file_name);
	int status = send_command(mySocket, m);
	if (status == 0) {
		char* buffer = (char*) calloc(MAX_FILE_SIZE, 1);
		if (!buffer) {
			printf("calloc error\n");
			return 0;
		}
		status = addFileClientSide(buffer, path_to_file); // buffer now has whole content of file

		if (buffer == NULL) {
			printf("maybe empty file. Not sent\n");
			free(path_to_file);
			return 0;
		}
		Message* fileContent = (Message*) malloc(sizeof(Message));
		createMessageCommand(fileContent, FILE_CONTENT, buffer);
		status = send_command(mySocket, fileContent);
		free(buffer);
		free(fileContent);
		if (status != 0) {
			printf("error, re-send message\n");
			free(m);
			return 0;
		}
	} else
		printf("Error, re-send message\n");
	free(m);
	return 0;
}


//FIX THIS//
int getFileCommand(Message* m, char* file_name, char* path_to_save,
		int mySocket) {
	createMessageCommand(m, GET_FILE, file_name);
	int status = send_command(mySocket, m);
	if (status != 0) {
		printf("error, re-send message\n");
		free(m);
		return 0;
	}
	free(m);
	return 0;
}


//TODO!!!!!!!!!!!//

/*int handleGetFileRecieve(Message* m) { //fix this
	int status = receive_command(mySocket, m);
	char* pathToFile = (char*) calloc(
			(strlen(path_to_save) + strlen(file_name)), sizeof(char));
	strcpy(pathToFile, path_to_save);
	strcpy(pathToFile + strlen(path_to_save), file_name);
	if (getFileClientSide(pathToFile, m->arg1))
		printf("File could not be opened\n");
	//todo
}*/

int sendClientCommand(char* commandStr, int mySocketfd) {
	Message* m = (Message*) malloc(sizeof(Message));
	char* c = malloc(strlen(commandStr) + 5);
	char delimit[] = " \t\r\n\v\f";
	strcpy(c, commandStr);
	c[strlen(commandStr) + 1] = '\0';
	char *str1 = strtok(c, delimit);
	char *str2 = strtok(NULL, delimit);
	if (str2 != NULL) {
		char *str3 = strtok(NULL, delimit);
		if (str3 != NULL) {
			if (strcmp(str1, "add_file") == 0) {
				if (addFileCommand(m, str2, str3, mySocketfd) == 1) {
					printf("Error in add file command %s\n", strerror(errno));
					free(c);
					free(m);
					return 1;
				}
				free(c);
				free(m);
				return 0;
			} else if (strcmp(str1, "get_file") == 0) {
				if (getFileCommand(m, str2, str3, mySocketfd) == 1) {
					printf("Error in get file command %s\n", strerror(errno));
					free(c);
					return 1;
				}
				free(c);
				return 0;
			}else if (strcmp(str1, "msg") == 0 ){
				chopN(commandStr, strlen(str1)+strlen(str2) + 2);
				if (sendMsgCommand(m, str2, commandStr, mySocketfd) == 1){
					printf("Error in add file command %s\n", strerror(errno));
					free(c);
					free(m);
					return 1;	
				}
				return 0;
			}
			else {
				printf("Error: Invalid command \n");
				free(m);
				free(c);
				return 0;
			}
		}
		if (strcmp(str1, "delete_file") == 0) {
			printf("in delete file\n");
			if (deleteFileCommand(m, str2, mySocketfd) == 1) {
				printf("Error in delete file command %s \n", strerror(errno));
				free(c);
				return 1;
			}
			free(c);
			return 0;
		}
	}
	if (strcmp(str1, "list_of_files") == 0) {
		if (listOfFilesCommand(m, commandStr, mySocketfd) == 1) {
			printf("Error in get list of files command %s \n", strerror(errno));
			free(c);
			return 1;
		}
		free(c);
		return 0;
	}
	if (strcmp(str1, "users_online") == 0) {
		if (listOfOnlineUsersCommand(m, commandStr, mySocketfd) == 1) {
			printf("Error in get online users list command %s \n",
					strerror(errno));
			free(c);
			return 1;
		}
		free(c);
		return 0;
	}
	if (strcmp(str1, "read_msgs") == 0) {
		if (readMessagesClient(m, mySocketfd) == 1) {
			printf("Error in get online users list command %s \n",
					strerror(errno));
			free(c);
			return 1;
		}
		free(c);
		return 0;
	} else if (strcmp(str1, "quit") == 0) {
		printf("we are in quit\n");
		createQuitCommand(m, mySocketfd);
		free(m);
		free(c);
		return 1; //quiting
	} else {
		printf("Invalid command \n");
	}
	free(c);
	free(m);
	return 0;
}

int sendMsgCommand(Message* m,char* arg2, char* arg3, int mySocketfd)
{
	char* buffer = (char*) malloc(sizeof(char) * (120));//macros for length
	if (buffer == 0){
		printf("Error. re-send message %s\n", strerror(errno));
		return -1;
	}
	buffer[120]='\0';
	strcpy(buffer, arg2);
	strcat(buffer, "\n");
	strcat(buffer, arg3);
	createMessageCommand(m, MSG, buffer);
	int status = send_command(mySocketfd, m);
	if (status != 0) {
		printf("Error. re-send message %s\n", strerror(errno));
		free(buffer);
		return 1;
	}
	free(buffer);
	return 0;
}


int addFileClientSide(char* buffer, char* filePath) {
	FILE* fp = fopen(filePath, "r");
	if (!fp) {
		printf("File cannot be opened \n");
		return 1;
	}
	fread(buffer, MAX_FILE_SIZE, 1, fp);
	fclose(fp);
	return 0;
}

int getFileClientSide(char* filePath, char* fileBuffer) {
	FILE *file = fopen(filePath, "w");
	if (!file) {
		printf("File Not Opened\n");
		return 1;
	}
	fwrite(fileBuffer, sizeof(char), MAX_FILE_SIZE, file);
	fclose(file);
	return 0;
}

int client_start(char* hostname, int port) {
	if (hostname == NULL) {
		char* hostname = (char*) malloc(sizeof(char) * 11);
		strcpy(hostname, "localhost");
		port = 1337;
	} else if (port == 0)
		port = 1337;

	int status;
	int socketfd = socket(AF_INET, SOCK_STREAM, 0);

	if (socketfd < 0) {
		printf("Could not create socket\n");
		return 1;
	}
	struct sockaddr_in server_addr;

	memset(&server_addr, '0', sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	inet_aton(hostname, &server_addr.sin_addr);
	if (connect(socketfd, (struct sockaddr*) &server_addr,
			sizeof(server_addr)) < 0) {
		close(socketfd);
		printf("Connection failed \n");
		return 1;
	}	
	Message* msg = (Message*) malloc(sizeof(Message));
	status = receive_command(socketfd, msg);
	if (status) {
		printf("problem in greeting\n");
	} else {
		printMessageArg(msg);
		status = defineUser(socketfd);
	}
	free(msg);
	char* inputStr;
	fd_set read_fds;
	while (status == 0) {
		build_fd_sets_client(socketfd, &read_fds);
		int activity = select(socketfd + 1, &read_fds, NULL, NULL, NULL);
		if (activity == -1) {
			//todo
			printf("in activity = -1\n");
			fflush(NULL);
		} else if (activity == 0) {
			printf("in activity = 0\n");
			fflush(NULL);
		}
		else // returns # of ready fd's
		{
	
			if (FD_ISSET(socketfd, &read_fds))
			{	//messages from server
				printf("receiving command from server\n");
				Message* m = (Message*) malloc(sizeof(Message));//free this //todo
				status = receive_command(socketfd, m);
				if (status) {
					printf("Error in receiving message %s\n", strerror(errno));
				} else
					status = handleServerMessage(m);	//check status
			}
			
			if (FD_ISSET(STDIN, &read_fds)) {	//commands from user
				inputStr = (char*) malloc(
						sizeof(char)
								* (MAX_COMMAND_NAME + 2 + 2 * MAX_ARG_LEN));
				fgets(inputStr, MAX_COMMAND_NAME + 2 + 2 * MAX_ARG_LEN, stdin);
				status = sendClientCommand(inputStr, socketfd);
				if (status == 1)
					printf("Error in sendClientCommand\n");
				free(inputStr);
			}
		}

	}
	if (close(socketfd) == -1) {
		printf("close() failed\n");
		free(inputStr);
		return 1;

	}
	return status;
}

void build_fd_sets_client (int socketfd, fd_set *read_fds)
{
  FD_ZERO(read_fds);
  FD_SET(STDIN, read_fds);
  FD_SET(socketfd, read_fds);

}


int handleServerMessage(Message* m) {
	switch (m->header.type) {//check this
	case GET_FILE:
		//handleGetFile();
		printMessageArg(m);
		return 0;
	case LIST_OF_FILES:
		printMessageArg(m);
		return 0;
	case DELETE_FILE:
		printMessageArg(m);
		return 0;
	case ADD_FILE:
		printMessageArg(m);
		return 0;
	case LIST_OF_ONLINE_USERS:
		printMessageArg(m);
		return 0;
	case MSG:
		printMessageArg(m);
		return 0;
	case READ_MSGS:
		printMessageArg(m);
		return 0;
	case LOGIN_DETAILS:
		printMessageArg(m);
		return 0;
	case GREETING:
		printMessageArg(m);
		return 0;
	default:
		return 1;
	}
	return 0;
}

