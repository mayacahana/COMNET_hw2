/*
 * server_protocol.c
 *
 *  Created on: Nov 10, 2017
 *      Author: mayacahana
 */

#include "server_protocol.h"

Message* createServerMessage(MessageType type, char* arg1) {
	Message* msg = (Message*) malloc(sizeof(Message));
	strcpy(msg->arg1, arg1);
	msg->header.arg1len = strlen(arg1) + 1;
	msg->header.type = type;
	return msg;
}

void freeUsers(User** usersArray, int numOfUsers) {
	for (int i = 0; i < numOfUsers; i++) {
		free(usersArray[i]->dir_path);
		free(usersArray[i]->password);
		free(usersArray[i]->user_name);
		free(usersArray[i]);
	}
	free(usersArray);
}

void addFile(int clientSocket, Message* msg, User* user) {
	if (!user || !msg) {
		printf("Error in Message or User");
		return;
	}
	char* pathToFile = (char*) calloc((strlen(user->dir_path)+strlen(msg->arg1)+5),sizeof(char));
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
	memset(msg->arg1, 0,MAX_FILE_SIZE);
	int status = receive_command(clientSocket, msg);
	if (status) {
		printf("Couldn't recve Buffer\n");
	}
	fwrite(msg->arg1, sizeof(char), MAX_FILE_SIZE, file); /////
	fclose(file);
	msgToSend = createServerMessage(msg->header.type, "File added\n");
	status = send_command(clientSocket, msgToSend);
	if (status){
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

void sendListOfFiles(int clientSocket, User* user) {
	if (!user) {
		return;
	}
	char files_names[MAX_FILES_PER_CLIENT * MAX_FILE_NAME] = { '\0' };
	char file_name[MAX_FILE_NAME];
	DIR *d;
	struct dirent *dir;
	int numOfFiles=0;
	d = opendir(user->dir_path);
	if (d != NULL) {
		while ((dir = readdir(d)) != NULL) {
			numOfFiles++;
			if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0){
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
	fp = fopen(pathToFile, "r");
	if (fp == NULL) {
		printf("Can't open the file to send");
		fflush(NULL);
		return;
	}
	fflush(NULL);
	//read the file into a buffer
	char* fileBuffer = (char*) malloc(sizeof(char) * MAX_FILE_SIZE + 1);
	if (!fileBuffer){
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
	msg->header.arg1len = strlen(fileBuffer);
	send_command(clientSocket, msg);
	free(fileBuffer);
}
int handleMessage(int clientSocket, Message *msg, User* user) {
	if (!msg) {
		return 1;
	}
	switch (msg->header.type) {
	case LIST_OF_FILES:
		sendListOfFiles(clientSocket, user);
		return 0;
	case DELETE_FILE:
		deleteFile(clientSocket, msg, user);
		return 0;
	case ADD_FILE:
		addFile(clientSocket, msg, user);
		return 0;
	case GET_FILE:
		sendFileToClient(clientSocket, msg, user);
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

/*
 * welcome the accepted socket, get username and password and checks if it fits
 * any user name in the users list *users. if not, allows client to quit or retype,
 * else, calls getNameAndFiles to greet the user.
 *
 */

int client_serving(int clientSocket, User **users, int numOfUsers) {
	User* user = NULL;
	Message *user_msg = (Message *) malloc(sizeof(Message));
	Message *pass_msg = (Message*) malloc(sizeof(Message));
	Message *response;
	char* nameandfile;
	int status = 1;
	while (status) {
		receive_command(clientSocket, user_msg);
		if (user_msg->header.type != QUIT) {
			receive_command(clientSocket, pass_msg);
			for (int i = 0; i < numOfUsers; i++) {
				if (strcmp(users[i]->user_name, user_msg->arg1) == 0) {
					if (strcmp(users[i]->password, pass_msg->arg1) == 0) {
						user = users[i];
						nameandfile = getNameAndFiles(user);
						response = createServerMessage(LOGIN_DETAILS,nameandfile);
						status = 0;
						i = numOfUsers;
					}
				}
			}
			if (user == NULL) {
				response = createServerMessage(INVALID_LINE, "Wrong username or/and password. please try again\n");
			}
			send_command(clientSocket, response);
			free(response);
			if(user != NULL){
				free(nameandfile);
			}
		} else {
			free(user_msg);
			free(pass_msg);
			return 0;
		}
	}
 	status = 0;
	while (!status && user_msg->header.type != QUIT) {
		receive_command(clientSocket, user_msg);
		status = handleMessage(clientSocket, user_msg, user);
	}
	free(user_msg);
	free(pass_msg);
	return 0;
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

void start_listen(User** usersArray, int numOfUsers, int port) {
	int status, newsocketfd;
	int socketfd = socket(PF_INET, SOCK_STREAM, 0);
	if (socketfd == -1) {
		printf("%s\n", strerror(errno));
		return;
	}
	struct sockaddr_in my_addr, client_addr;
	socklen_t client_size = sizeof(client_addr);
	if (socketfd < 0) {
		printf("Could not create socket\n");
		return;
	}
	bzero((char *) &my_addr, sizeof(my_addr));
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	my_addr.sin_family = AF_INET;
	status = bind(socketfd, (struct sockaddr *) &my_addr, sizeof(my_addr));
	if (status < 0) {
		printf("Bind error: %s\n", strerror(errno));
		freeUsers(usersArray, numOfUsers);
		return;
	}
	// MAX users is 1
	status = listen(socketfd, 1);
	if (status < 0) {
		close(socketfd);
		printf("The listen() failed: %s\n", strerror(errno));
		freeUsers(usersArray, numOfUsers);
		return;
	}
	// keep accepting users, one at a time
	while (1) {
		newsocketfd = accept(socketfd, (struct sockaddr *) &client_addr,
				&client_size);
		if (newsocketfd < 0) {
			printf("accept() not successful...");
			printf("%s\n", strerror(errno));
			return;
		}
		sendGreetingMessage(newsocketfd);
		client_serving(newsocketfd, usersArray, numOfUsers);
	}
}

void start_server(char* users_file, const char* dir_path, int port) {
	//creating the main folder
	//reading input file
	FILE* usersFile = fopen(users_file, "r");
	if (!usersFile) {
		printf("The file: %s couldn't be opened \n", users_file);
		return;
	}
	User** usersArray = (User**) malloc(15 * sizeof(User));
	memset(usersArray, 0, sizeof(User) * 15);
	int numOfUsers = 0;

	if (usersFile != NULL) {
		char* user_buffer = (char*) malloc(sizeof(char) * 26);
		char* pass_buffer = (char*) malloc(sizeof(char) * 26);
		char* fileDirPath = (char*) malloc(sizeof(char)*(strlen(dir_path)+sizeof(user_buffer)+5));
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
				usersArray[numOfUsers] = newUser;
				numOfUsers++;
			} else {
				printf("Error: %s \n", strerror(errno));
				printf("cannot create user directory for :%s\n", user_buffer);
			}
			user_buffer = (char*) malloc(sizeof(char) * 26);
			pass_buffer = (char*) malloc(sizeof(char) * 26);
			fileDirPath = (char*) malloc(sizeof(char)*(strlen(dir_path) + sizeof(user_buffer)+5));
		}
		start_listen(usersArray, numOfUsers, port);
	}
}
