/*
 * server_protocol.c
 *
 *  Created on: Nov 10, 2017
 *      Author: mayacahana
 */

#include "server_protocol.h"

// global Users Array
User** usersArray;
int listen_socket;
int numOfUsers;
connection_t connection_users[MAX_CLIENTS];

Message* createServerMessage(MessageType type, char* arg1) {
	Message* msg = (Message*) malloc(sizeof(Message));
	strcpy(msg->arg1, arg1);
	msg->header.arg1len = strlen(arg1) + 1;
	msg->header.type = type;
	return msg;
}

void freeUsers(int numOfUsers) {
	for (int i = 0; i < numOfUsers; i++) {
		free(usersArray[i]->dir_path);
		free(usersArray[i]->password);
		free(usersArray[i]->user_name);
		free(usersArray[i]);
	}
	free(usersArray);
}

User* getUser(char* username) {
	printf("in getUser\n\nusername = %s\n", username);
	if (username == NULL){
		printf("username is NULL\n");
		return NULL;
	}
	int i = 0;
	User** selectedUser = usersArray;
	while (selectedUser[i] != NULL) {
		if (strcmp(selectedUser[i]->user_name, username) == 0){
			printf("selectedUser[%d]->user_name = %s\n", i, selectedUser[i]->user_name);
			return selectedUser[i];
		}
		i++;
	}
	return NULL;
}

void addFile(int clientSocket, Message* msg, User* user) {
	if (!user || !msg) {
		printf("Error in Message or User");
		return;
	}
	char* pathToFile = (char*) calloc(
			(strlen(user->dir_path) + strlen(msg->arg1) + 5), sizeof(char));
	strcpy(pathToFile, user->dir_path);
	pathToFile[strlen(user->dir_path)] = '/';
	strcpy(pathToFile + strlen(user->dir_path) + 1, msg->arg1);
	FILE *file = fopen(pathToFile, "w");
	Message* msgToSend;
	if (file == NULL) {
		printf("File Not Added\n");
		msgToSend = createServerMessage(ERROR,
				"couldn't open file in server side\n");
		send_command(clientSocket, msgToSend);
		free(pathToFile);
		free(msgToSend);
		return;
	}
	memset(msg->arg1, 0, MAX_FILE_SIZE);
	int status = receive_command(clientSocket, msg);
	if (status) {
		printf("Couldn't recve Buffer\n");
	}
	fwrite(msg->arg1, sizeof(char), MAX_FILE_SIZE, file); /////
	fclose(file);
	msgToSend = createServerMessage(msg->header.type, "File added\n");
	status = send_command(clientSocket, msgToSend);
	if (status) {
		printf("Could not send File added\n");
	}
	free(msgToSend);
	free(pathToFile);
}

void deleteFile(int clientSocket, Message* msg, User* user) {
	if (!user || !msg) {
		printf("Error in Message or User");
		return;
	}
	char pathToFile[sizeof(user->dir_path) + sizeof(msg->arg1) + 5];
	strcpy(pathToFile, user->dir_path);
	strcat(pathToFile, "/");
	strcat(pathToFile, msg->arg1);
	int status = remove(pathToFile);
	char* arg = (char*) malloc(sizeof(char) * 20);
	if (status == 0) {
		strcpy(arg, "File removed!\n");
	} else {
		strcpy(arg, "No such file exists!\n");
	}
	Message* msgToSend = createServerMessage(DELETE_FILE, arg);
	send_command(clientSocket, msgToSend);
	free(arg);
	free(msgToSend);
}

void readMessages(int clientSocket, User* user) {
	
	if (!user) {
		printf("Error in read messages");
		return;
	}
	Message* msg = createServerMessage(GET_FILE,"Messages_received_offline.txt");
	sendFileToClient(clientSocket, msg, user);
	// delete the content from the file
	char* pathToFile = (char*) calloc(((strlen)(user->dir_path) + strlen("Messages_received_offline.txt")+ 5), sizeof(char));
	strcpy(pathToFile, user->dir_path);
	pathToFile[strlen(user->dir_path)] = '/';
	strcpy(pathToFile + strlen(user->dir_path) + 1,"Messages_received_offline.txt");
	fclose(fopen(pathToFile, "w"));
	free(pathToFile);
	free(msg);
	return;
}


void messageOtherUser(int clientSocket, Message* msg, User* user) {
	if (!user) {
		printf("Error in read messages");
		return;
	}
	char buffer[500];//fix macro size
	strcpy(buffer, msg->arg1);
	printf("arg1 in msg ither user: %s\n" ,msg->arg1);
	fflush(stdout);
	const char* delimit = "\n";
	char *username = strtok(buffer, delimit);
	char *sentfrom = user->user_name;
	char *message_content= strtok(NULL, delimit);
	char *full_message = (char*) malloc(sizeof(char)*(strlen("New message from: ") + MAX_USERNAME_SIZE + MAX_MSG_CONTENT + 10));
	full_message[strlen("New message from: ") + MAX_USERNAME_SIZE +10] = '\0';
	strcpy(full_message, "New message from ");
	strcat(full_message, sentfrom);
	strcat(full_message, ": ");
	strcat(full_message, message_content);
	strcat(full_message, "\n");
	printf("full: %s \n", full_message);
	fflush(stdout);
	User** users = usersArray;
	printf("numofusers: %d\n", numOfUsers);
	username[strlen(username)-1] = '\0'; 
	for(int i =0; i < numOfUsers; i++) {
		printf("in for befor strcmp\n");
		if(strcmp(users[i]->user_name, username) == 0) {
			printf("username(to send to)%s \n", username);
			fflush(stdout);
			msg = createServerMessage(MSG, full_message);	
			int socket_to_send = -1;
			for (int j = 0; j < MAX_CLIENTS; j++){
				if(connection_users[j].username != NULL){
					if(strcmp(connection_users[j].username, username) == 0){
						socket_to_send = connection_users[j].socket;
						printf("socket number = %d\n", socket_to_send);
						fflush(stdout);
					}
				}
			}
			printf("found the user to send \n");
			fflush(stdout);
			if (socket_to_send < 0) { //not online
				//TODO: write to txt file
				full_message = (char*) calloc((strlen("Message received from: ") + MAX_MSG_CONTENT+ MAX_USERNAME_SIZE + 10),sizeof(char));
				//full_message[strlen("Message received from: ")+MAX_USERNAME_SIZE+10] = '\0';
				strcpy(full_message, "Message received from ");
				strcpy(full_message+22, sentfrom);
				strcpy(full_message+22+strlen(sentfrom), ": ");
				strcpy(full_message+24+strlen(sentfrom), message_content);
				//printf("strlen content = %d\n", strlen(message_content));
				printf("strlen = %d\n", strlen(full_message));
				full_message[strlen(full_message)] = '\n';
				char* pathToFile = (char*) calloc(((strlen)(users[i]->dir_path) + strlen("Messages_received_offline.txt") + 5), sizeof(char));
				strcpy(pathToFile, users[i]->dir_path);
				pathToFile[strlen(users[i]->dir_path)] = '/';
				strcpy(pathToFile + strlen(users[i]->dir_path) + 1,"Messages_received_offline.txt");
				FILE* msg_offline = fopen(pathToFile, "a");
				if (msg_offline != NULL) {
					fwrite(full_message, sizeof(char), MAX_MSG_CONTENT, msg_offline);
					fclose(msg_offline);
				} else {
					printf("Error opening offline msgs file");
				}
			} else if (send_command(socket_to_send, msg) != 0) { // the user is online
				printf("problem in sending file from MESSAGE OTHER USER\n");
			}
			break;
		}
	}
	free(msg);
}





void sendListOfFiles(int clientSocket, User* user) {
	printf("in sendListOfFiles\n");
	if (!user) {
		return;
	}
	char files_names[MAX_FILES_PER_CLIENT * MAX_FILE_NAME] = { '\0' };
	char file_name[MAX_FILE_NAME];
	DIR *d;
	struct dirent *dir;
	int numOfFiles = 0;
	d = opendir(user->dir_path);
	if (d != NULL) {
		while ((dir = readdir(d)) != NULL) {
			numOfFiles++;
			if (strcmp(dir->d_name, ".") != 0
					&& strcmp(dir->d_name, "..") != 0) {
				sprintf(file_name, "%s\n", dir->d_name);
				strcat(files_names, file_name);
			}
		}
		closedir(d);
	} else {
		printf("Wrong dir path for user %s \n", user->user_name);
	}
	if (numOfFiles <= 2)
		strcpy(files_names, "No Files in your directory\n");
	Message* msg = createServerMessage(LIST_OF_FILES, files_names);
	printf("file_names = %s", files_names);
	send_command(clientSocket, msg);
	free(msg);
	return;
}

void sendListOfOnlineUsers(int clientSocket, User* user) {
	if (!user) {
		return;
	}
	char online_users[MAX_CLIENTS * MAX_USERNAME_SIZE + strlen("online users: ")];
	char user_name[MAX_USERNAME_SIZE];
	User** users_array = usersArray;
	strcpy(online_users, "online users: ");
	int cnt = 0;
	while (users_array[cnt] != NULL) {
		if (users_array[cnt]->online == 1) {
			sprintf(user_name, "%s ", users_array[cnt]->user_name);
			strcat(online_users, user_name);
		}
		cnt++;
	}
	strcat(online_users, "\n");
	Message* msg = createServerMessage(LIST_OF_ONLINE_USERS, online_users);
	send_command(clientSocket, msg);
	free(msg);
	return;
}

void sendFileToClient(int clientSocket, Message* msg, User* user) {
	// get the file name
	FILE* fp;
	char pathToFile[strlen(user->dir_path) + strlen(msg->arg1) + 1];
	sprintf(pathToFile, "%s/%s", user->dir_path, msg->arg1);
	puts(pathToFile);
	fp = fopen(pathToFile, "rb");
	if (fp == NULL) {
		printf("Can't open the file to send");
		fflush(NULL);
		return;
	}
		//read the file into a buffer
	char* fileBuffer = (char*) malloc(sizeof(char) * MAX_FILE_SIZE + 1);
	if (!fileBuffer) {
		printf("no buffer\n");
		fflush(NULL);
		return;
	}
	fread(fileBuffer, MAX_FILE_SIZE, 1, fp);
	fflush(NULL);
	fclose(fp);
	fflush(NULL);
	strcpy(msg->arg1, fileBuffer);
	fflush(NULL);
	printf("send file: %s\n", fileBuffer);
	msg->header.arg1len = strlen(fileBuffer);
	send_command(clientSocket, msg);
	free(fileBuffer);
}

int handleMessage(int clientSocket, Message* msg, User* user) {
	if (!msg) {
		return 1;
	}
	switch (msg->header.type) {
	case LIST_OF_FILES:
		printf("in list_of_files case in handlMessage\n\n");
		sendListOfFiles(clientSocket, user);
		return 0;
	case DELETE_FILE:
		printf("in delete file case in handlMessage\n");
		deleteFile(clientSocket, msg, user);
		return 0;
	case ADD_FILE:
		printf("in add file case in handlMessage\n");
		addFile(clientSocket, msg, user);
		return 0;
	case GET_FILE:
		printf("in get file case in handlMessage\n");		
		sendFileToClient(clientSocket, msg, user);
		return 0;
	case LIST_OF_ONLINE_USERS:
		printf("in list of online users case in handlMessage\n");
		sendListOfOnlineUsers(clientSocket, user);
		return 0;
	case READ_MSGS:
		printf("in read msgs case in handlMessage\n");
		readMessages(clientSocket, user);
		return 0;
	case MSG:
		printf("in msg case in handlMessage\n");
		messageOtherUser(clientSocket, msg, user);
		return 0;
	default:
		return 1;
	}
}

char* getNameAndFiles(User* user) {
	if (!user) {
		return NULL;
	}
	int numOfFiles = 0;
	DIR *d = opendir(user->dir_path);
	struct dirent* dir;
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			numOfFiles++;
		}
		closedir(d);
	} else {
		return NULL;
	}
	char* arg = (char*) malloc(sizeof(char) * (MAX_USERNAME_SIZE + 50));
	sprintf(arg, "Hi %s, you have %d files stored.\n", user->user_name,
			(numOfFiles - 2));
	return arg;
}

void sendGreetingMessage(int clientSocket) {
	Message* greeting = createServerMessage(GREETING,
			"Welcome! Please log in.\n");
	int status = send_command(clientSocket, greeting);
	if (status) {
		printf("Error in greeting\n");
	}
	free(greeting);
}

int build_fd_sets(fd_set *read_fds) {
	int i;
	FD_ZERO(read_fds);
	FD_SET(listen_socket, read_fds);
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (connection_users[i].socket != -1) {
			FD_SET(connection_users[i].socket, read_fds);
		}
	}
	return 0;
}

int login(int clientSocket) {
	User* user = NULL;
	Message* msg = (Message *) malloc(sizeof(Message));
	Message *response;
	char* nameandfile;
	receive_command(clientSocket, msg);
	//printf("msg->arg1 =%s \n", msg->arg1);
	char login_details[MAX_PASSWORD_SIZE + MAX_USERNAME_SIZE + 1];
	strcpy(login_details, msg->arg1);
	const char* delimit = "\n";
	char *username = strtok(login_details, delimit);
	char *password = strtok(NULL, delimit);
	if (msg->header.type != QUIT) {
		int i = 0;
		User ** users_array = usersArray;
		while (users_array[i] != NULL) {
			if (strcmp(users_array[i]->user_name, username) == 0) {
				if (strcmp(users_array[i]->password, password) == 0) {
					user = users_array[i];
					nameandfile = getNameAndFiles(user);
					response = createServerMessage(LOGIN_DETAILS, nameandfile);
					// online user- on
					user->online = 1;
					// insert to connection_users
					int j;
					for (j=0; j<MAX_CLIENTS; j++) {
						if (connection_users[j].socket == clientSocket){
							connection_users[j].username = users_array[i]->user_name;
							printf("user %s connected \n",connection_users[j].username);
						}
					}
					break;
				}
			}
			i++;
		}
	}
	if (user == NULL) {
		response = createServerMessage(INVALID_LINE,
				"Wrong username or/and password. please try again\n");
	}
	send_command(clientSocket, response);
	free(response);
	if (user != NULL) {
		free(nameandfile);
	}
	free(msg);
	return user == NULL ? 0 : -1;
}
// this function get a user struct and create a connection between
// him and the server... 
int accept_new_connection() {
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(client_addr));
	socklen_t client_len = sizeof(client_addr);
	int new_client_soc;
	new_client_soc = accept(listen_socket, (struct sockaddr *) &client_addr,
			&client_len);
	if (new_client_soc < 0) {
		perror("accept()");
		return -1;
	}
	int i;
	char client_ipv4_str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &client_addr.sin_addr, client_ipv4_str, INET_ADDRSTRLEN);
	printf("Incoming connection from %s: %d.\n", client_ipv4_str,
			client_addr.sin_port);
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (connection_users[i].socket == -1) {
			connection_users[i].socket = new_client_soc;
			return new_client_soc;
		}
	}
	printf("There are too much connections. closing this connection. \n");
	close(new_client_soc);
	return -1;
}
// closing the client connection
int close_client(connection_t* client) {
	close(client->socket);
	return 0;
}

void start_listen(int numOfUsers, int port) {
	int status;
	// start listen socket
	listen_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (listen_socket == -1) {
		printf("%s\n", strerror(errno));
		return;
	}
	struct sockaddr_in my_addr;
	//socklen_t client_size = sizeof(client_addr);
	bzero((char *) &my_addr, sizeof(my_addr));
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	my_addr.sin_family = AF_INET;
	status = bind(listen_socket, (struct sockaddr *) &my_addr, sizeof(my_addr));
	if (status < 0) {
		printf("Bind error: %s\n", strerror(errno));
		freeUsers(numOfUsers);
		return;
	}
	// MAX_CLIENTS users is 15
	status = listen(listen_socket, MAX_CLIENTS);
	if (status < 0) {
		close(listen_socket);
		printf("The listen() failed: %s\n", strerror(errno));
		freeUsers(numOfUsers);
		return;
	}
	printf("Accepting connections on port %d.\n", (int) port);

	// init the connection_user array !
	int i;
	for (i = 0; i < MAX_CLIENTS; i++) {
		connection_users[i].socket = -1;
		connection_users[i].username = NULL;
	}
	// init fds
	fd_set read_fds;
	int high_socket = listen_socket;
	while (1) {
		high_socket = listen_socket;
		int i;
		FD_ZERO(&read_fds);
		FD_SET(listen_socket, &read_fds);
		for (i = 0; i < MAX_CLIENTS; i++) 
		{
			if (connection_users[i].socket != -1)
			{
				FD_SET(connection_users[i].socket, &read_fds);
				if (connection_users[i].socket > high_socket)
				{
					high_socket = connection_users[i].socket;
				}
			}
		}
		/* find the max socket
		for (i = 0; i < MAX_CLIENTS; i++) {
			//printf("connection_users[%d].socket = %d\n", i, connection_users[i].socket);
			if (connection_users[i].socket > high_socket) {
				high_socket = connection_users[i].socket;
			}
		}*/
		int activity, client_sock;
		activity = select(high_socket + 1, &read_fds, NULL, NULL, NULL);
		switch (activity) {
		case -1:
			perror("select() failed");
			printf("in case 1\n"); //delete this
			break;
			//TODO: closing everything
		case 0:
			perror("select() return 0");
			printf("in case 0\n"); //delete this
			break;
			//TODO: closing everything
		default:
			if (FD_ISSET(listen_socket, &read_fds)) {
				client_sock = accept_new_connection();
				printf("socket: %d has been connected\n", client_sock);
				if (client_sock > -1) {
					sendGreetingMessage(client_sock);
				}
			}
			
			Message* user_msg = (Message *) malloc(sizeof(Message));
			int status;
			for (i = 0; i < MAX_CLIENTS; i++) {
				//printf("connection_users[%d].socket = %d\n",i, connection_users[i].socket);
				if ((connection_users[i].socket != -1) && (FD_ISSET(connection_users[i].socket, &read_fds)) != 0) {
					if (connection_users[i].username != NULL) {
						printf("receiving message from %s socket: %d\n", connection_users[i].username, connection_users[i].socket);
						receive_command(connection_users[i].socket, user_msg);
						printf("user_msg->arg1 = %s\n", user_msg->arg1);
						status = handleMessage(connection_users[i].socket,user_msg,getUser(connection_users[i].username)); 
						if (status != 0) {
							perror("Error in handle message");
						}
					} else {
						printf("before login\n");
						login(connection_users[i].socket);
					}
				}

			}

		}
		// newsocketfd = accept(socketfd, (struct sockaddr *) &client_addr,
		// 		&client_size);
		// if (newsocketfd < 0) {
		// 	printf("accept() not successful...");
		// 	printf("%s\n", strerror(errno));
		// 	return;
		// }
		// sendGreetingMessage(newsocketfd);
		// client_serving(newsocketfd, numOfUsers);
	}
	
	printf("server out of while loop\n");
}

void start_server(char* users_file, const char* dir_path, int port) {
	//creating the main folder
	//reading input file
	FILE* usersFile = fopen(users_file, "r");
	if (!usersFile) {
		printf("The file: %s couldn't be opened \n", users_file);
		return;
	}
	usersArray = (User**) malloc(15 * sizeof(User));
	User** users_array = usersArray;
	memset(usersArray, 0, sizeof(User) * 15);
	numOfUsers = 0;

	if (usersFile != NULL) {
		char* user_buffer = (char*) malloc(sizeof(char) * 26);
		char* pass_buffer = (char*) malloc(sizeof(char) * 26);
		char* fileDirPath = (char*) malloc(
				sizeof(char) * (strlen(dir_path) + sizeof(user_buffer) + 5));
		while (fscanf(usersFile, "%s\t", user_buffer) > 0) {
			strcpy(fileDirPath, dir_path);
			fileDirPath[strlen(dir_path)] = '/';
			strcpy(fileDirPath + strlen(dir_path) + 1, user_buffer);
			if (!mkdir(fileDirPath, ACCESSPERMS)) {
				User* newUser = (User*) malloc(sizeof(User));
				newUser->user_name = user_buffer;
				fscanf(usersFile, "%s\n", pass_buffer);
				newUser->password = pass_buffer;
				newUser->dir_path = fileDirPath;
				newUser->online = 0;
				users_array[numOfUsers] = newUser;
				numOfUsers++;
				// create Messages_received_offline.txt file in his folder
				char* pathToFile = (char*) calloc(
						((strlen)(newUser->dir_path)
								+ strlen("Messages_received_offline.txt") + 5),
						sizeof(char));
				strcpy(pathToFile, newUser->dir_path);
				pathToFile[strlen(newUser->dir_path)] = '/';
				strcpy(pathToFile + strlen(newUser->dir_path) + 1,
						"Messages_received_offline.txt");
				FILE *fp = fopen(pathToFile, "a");
				if (!fp) {
					printf(
							"coudn't create the - Messages_received_offline.txt \n");
					return;
				}
				fclose(fp);
				free(pathToFile);
			} else {
				printf("Error: %s \n", strerror(errno));
				printf("cannot create user directory for :%s\n", user_buffer);
			}
			user_buffer = (char*) malloc(sizeof(char) * 26);
			pass_buffer = (char*) malloc(sizeof(char) * 26);
			fileDirPath = (char*) malloc(
					sizeof(char)
							* (strlen(dir_path) + sizeof(user_buffer) + 5));
		}
		start_listen(numOfUsers, port);
	}
}
