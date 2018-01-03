/*
 * file_server.c
 *
 *  Created on: 10 ���� 2017
 *      Author: ���
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define  DEF_PORT 1337
#include "server_protocol.h"

int main(int argc, char* argv[]) {
	if (argc < 3 || argc > 4) {
		printf("Invalid Argument\n");
		return 1;
	}
	struct stat dirctry;
	char* users_file = argv[1];
	char* dir_path = argv[2];
	int port = DEF_PORT;
	if (argc == 4) {
		port = atoi(argv[3]);
	}

	if (stat(dir_path, &dirctry) < 0) {
		printf("Directory doesn't exist or can not be opened\n");
		return 1;
	}
	if (!S_ISDIR(dirctry.st_mode)) {
		printf("%s is not a directory", dir_path);
		return 1;
	}
	start_server(users_file, dir_path, port);

	return 0;

}

