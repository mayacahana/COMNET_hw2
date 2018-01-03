/*
 * network.c
 *
 *  Created on: Nov 14, 2017
 *      Author: mayacahana
 */
#include "network.h"


void printMessageArg(Message* msg){
//	while(msg->arg1[i] != '\n'){
//		printf("%c", msg->arg1[i++]);
//	}
	printf("%s",msg->arg1);
	//printf("\n");
	fflush(NULL);
}


int send_command(int sckt, Message* msg_to_send) {
	int len = HEADER_SIZE + msg_to_send->header.arg1len;

	Message network_msg;
	network_msg.header.type = htons(msg_to_send->header.type);
	network_msg.header.arg1len = htons(msg_to_send->header.arg1len);
	strcpy(network_msg.arg1, msg_to_send->arg1);
	int total = 0;
	int bytesLeft = len;
	int n;
	while (total < len) {
		n = send(sckt, &network_msg+total,  bytesLeft, 0);
		if (n < 0) {
			printf("Error in send: %s\n", strerror(errno));
			break;
		}
		total += n;
		bytesLeft -= n;
	}
	return n == -1 ? -1 : 0;
}



int receiveAll(int socket, void* buffer, int* len) {
	int total = 0;
	int bytesLeft = *len;
	int n;
	while (total < *len) {
		n = recv(socket, (void*) ((long)buffer + total), (size_t) bytesLeft, 0);
		if (n < 0) {
			printf("Error in recv: %s\n", strerror(errno));
			break;
		}
		total += n;
		bytesLeft -= n;
	}
	*len = total;
	return n == -1 ? -1 : 0;
}

int receive_command(int sckt, Message* msg_received) {
	//recieve the Messageeader first
	int len_header = sizeof(MessageHeader);
	if (receiveAll(sckt, &msg_received->header, &len_header)) {
		printf("%s\n", strerror(errno));
		printf("Bytes recieved: %d \n", len_header);
		return 1;
	}
	msg_received->header.arg1len = ntohs(msg_received->header.arg1len);
	msg_received->header.type = ntohs(msg_received->header.type);
	if ((msg_received->header.arg1len)
			> MAX_DATA_SIZE) {
		printf("Recieve command failed- msg too big");
		return 1;
	}
	//receive arg1
	int len_arg1 = msg_received->header.arg1len;
	if (receiveAll(sckt, msg_received->arg1, &len_arg1)) {
		printf("%s\n", strerror(errno));
		printf("Bytes recieved: %d \n", len_header);
		return 1;
	}
	return 0;

}
