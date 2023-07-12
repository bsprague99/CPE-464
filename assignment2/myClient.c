/******************************************************************************
* myClient.c
*
******************************************************************************/
//e should be 3 bytes and its 1024
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdint.h>

#include "networks.h"
#include "recvAndSend.h"
#include "pollLib.h"
#include "handle_table.h"

#define MAXBUF 1024
#define DEBUG_FLAG 1
#define FLAG_M 5
#define FLAG_1 1
#define FLAG_10 10

void interact_with_server(int socketNum, int argc, char *argv[]);
int readFromStdin(char * buffer);
void checkArgs(int argc, char *argv[]);
void message_packet(int socketNum, int argc, char *argv[]);
void initial_packet(int socket, char *handle);
void print_handle(struct handle *handle);
void list_packet(int socketNum, char *handle);
void parse_chat_header(struct header header, int socketNum);
void parse_flag_5_client(struct header header, int socketNum);
void parse_flag_11(struct header header, int socketNum);
void broadcast_packet(int socketNum, int argc, char *argv[]);
void exit_packet(int socketNum, int argc, char *argv[]);
void parse_flag_9(struct header header, int socketNum);
void parse_flag_4_client(struct header header, int socketNum);


int main(int argc, char * argv[])
{
	int socketNum = 0;         //socket descriptor

	checkArgs(argc, argv);

	//check for a longer client name
	if(strlen(argv[1]) > 100)
	{
		printf("Client handle must be less than 100 characters.\n");
		exit(0);
	}
	/* set up the TCP Client socket  */
	socketNum = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);
	addToPollSet(STDIN_FILENO);
	initial_packet(socketNum, argv[1]);
	printf("$:");
	fflush(stdout);
	interact_with_server(socketNum, argc, argv);
	
	close(socketNum);
	
	return 0;
}

void initial_packet(int socket, char *handle)
{
	uint8_t HANDLE_LEN = strlen(handle);
	uint8_t sendBuff[MAXBUF];

	int length;
	struct header header;

	header.len = htons(4 + HANDLE_LEN);
	//printf("%d", header.len);
	header.flag = FLAG_1;
	fflush(stdout);

	memcpy(sendBuff, &header, sizeof(header));
	memcpy(sendBuff + sizeof(header), &HANDLE_LEN, sizeof(uint8_t));
	memcpy(sendBuff + 1 + sizeof(header), handle, HANDLE_LEN);
	
	addToPollSet(socket);

	if ((length = send(socket, sendBuff, ntohs(header.len), 0)) < 0) {
        perror("error in send");
        // return -1;
        exit(-1);
    }
	if ((length = recv(socket, &header, sizeof(struct header), MSG_WAITALL)) < 0)
	{
		perror("Error in payload");
		// return -1;
		exit(-1);
	}

	if (length == 0)
	{
		printf("Server socket has terminated\n");
		removeFromPollSet(socket);
		exit(1);
	}
	// if ((length = recv(socket, &handle, ntohs(header.len) - 3, MSG_WAITALL)) < 0)
	// {
	// 	perror("Error in payload");
	// 	// return -1;
	// 	exit(-1);
	// }

	if (header.flag == 3)
	{
		printf("Error: handle already taken\n");
		exit(1);
	}
}

void interact_with_server(int socketNum, int argc, char *argv[])
{
	char textBuf[MAXBUF];   //data buffer
	int length;
	char *token;
	char m = 'm';
	char M = 'M';
	char l = 'l';
	char L = 'L';
	char b = 'b';
	char B = 'B';
	char e = 'e';
	char E = 'E';
	int check_socket;
	struct header header;

	
	while (1)
	{
		if ((check_socket = pollCall(-1)) != -1)
		{
			if (check_socket == STDIN_FILENO)
			{
				printf("$:");

				//sendbuf will get packed
				readFromStdin(textBuf);
				token = strtok(textBuf, " ");
				if (token == NULL) 
				{
					continue;
				}

				if  (token[0] == '%')  
				{
					if ((token[1] == m) || (token[1] == M))
					{
						message_packet(socketNum, argc, argv);
					}
					else if ((token[1] == l) || (token[1] == L))
					{
						list_packet(socketNum, argv[1]);
					}
					else if ((token[1] == b) || (token[1] == B))
					{
						broadcast_packet(socketNum, argc, argv);
					}
					else if ((token[1] == e) || (token[1] == E))
					{
						exit_packet(socketNum, argc, argv);
					}
				}
			}
			else if (check_socket == socketNum)
			{
				fflush(stdout);
				//first recv is only the header
				if ((length = recv(socketNum, &header, sizeof(struct header), MSG_WAITALL)) < 0)
				{
					perror("recv call");
					exit(-1);
				}
				//printf("%d", header.flag);
				if (length == 0) {
					printf("server has closed\n");
					exit(1);
				}
				//check server flag in header and do switch
				parse_chat_header(header, socketNum);
				fflush(stdout);
				//second recv is the payload
			}
		}
	}
}

void parse_chat_header(struct header header, int socketNum)
{
	uint8_t flag = header.flag;

	switch(flag){
      	case 2:
        	break;
		case 4:
         	parse_flag_4_client(header, socketNum);
			break;
      	case 5:
         	parse_flag_5_client(header, socketNum);
		 	break;
      	case 11:
         	parse_flag_11(header, socketNum);
         	break;
		case 9:
			parse_flag_9(header, socketNum);
			break;

	}
}

void parse_flag_9(struct header header, int socketNum)
{
	if (header.flag == 9)
	{
		removeFromPollSet(socketNum);
		close(socketNum);
		printf("Client has exited chat.\n");
		exit(0);
	}
}

void exit_packet(int socketNum, int argc, char *argv[])
{
	struct header header;
	int send_len = 0;
	int recv_length;
	struct header response;

	header.flag = 8;
	header.len =  htons(3);

	if ((send_len = send(socketNum, &header, 3, 0)) < 0) 
	{
        perror("error in send");
        exit(-1);
    }
	if ((recv_length = recv(socketNum, &response, sizeof(struct header), MSG_WAITALL)) < 0)
	{
        perror("Error in the recv");
        exit(-1);
    }
	parse_chat_header(response, socketNum);
}

void broadcast_packet(int socketNum, int argc, char *argv[])
{
	// The format for a broadcast packet is: 
	// o Normal 3 byte chat-header (length, flag) (flag = 4) 
	// o 1 byte containing the length of the sending clients handle 
	// o Handle name of the sending client (no nulls or padding allowed) 
	// o Text message (a null terminated string) 
	// o This packet type does not have a destination handle since the packet should be 
	// forwarded to all other clients. 

	uint8_t sender_handle_len;
	char *token;
	uint16_t total = 3;
	char sendBuff[MAXBUF] = {'\0'};
	int length;

	sendBuff[2] = 4;
	
	sender_handle_len = (uint8_t) strlen(argv[1]);
	sendBuff[total] = sender_handle_len;
	total+= 1;
	memcpy(sendBuff + total, argv[1], sender_handle_len);
	total += sender_handle_len;

	//points to the entire text until the newline is pressed.
	token = strtok(NULL, "\n");
	if (token == NULL)
	{
		printf("Bad strtok()\n");
	}

	int count = strlen(token);
	int new_total = total;
	int data_sent = 0;
 
	//text message is larger than 200 bytes
	while (count >= 199)
	{
		memcpy(sendBuff + total, token + data_sent, 199);
		data_sent = 199 + data_sent;

		total = htons(total + 200);

		//new total pdu len
		memcpy(sendBuff, &total, 2);

		//flag
		sendBuff[2] = 4;

		count -= 199;

		if ((length = send(socketNum, sendBuff, ntohs(total), 0)) < 0) 
		{
			perror("error in send");
			exit(-1);
		}
		total = new_total;
	}
	// if msg is shorter than 200
	memcpy(sendBuff + total, token + data_sent, count);
	total =  htons(total + count);
	
	//new total pdu len
	memcpy(sendBuff, &total, 2);

	//flag
	sendBuff[2] = 4;

	//send the packet out to server
	if ((length = send(socketNum, sendBuff, ntohs(total), 0)) < 0) 
	{
		perror("error in send");
		exit(-1);
	}
	fflush(stdout);
}

void parse_flag_4_client(struct header header, int socketNum)
{
	uint8_t recvbuff[MAXBUF] = {'\0'};
	struct handle *handle;

	fflush(stdout);
	if (recv(socketNum, recvbuff, ntohs(header.len) - 3, MSG_WAITALL) < 0)
	{
		perror("Error in payload");
		// return -1;
		exit(-1);
	}
	handle = (struct handle *)recvbuff;
	print_handle(handle);
	printf(": %s\n$:", recvbuff + handle->len + 1);
	fflush(stdout);
}

void parse_flag_5_client(struct header header, int socketNum)
{
	uint8_t recvbuff[MAXBUF] = {'\0'};
	struct handle *handle;
	int index;
	char number_of_handles;
	int i;

	if (recv(socketNum, recvbuff, ntohs(header.len) - 3, MSG_WAITALL) < 0)
	{
		perror("Error in payload");
		// return -1;
		exit(-1);
	}

	handle = (struct handle *)recvbuff;
	index = handle->len + 1;
	print_handle(handle);
	number_of_handles = recvbuff[index];

	index += 1;
	fflush(stdout);

	for (i = 0; i < number_of_handles; i++)
	{
		handle = (struct handle *)(recvbuff + index);
		index += handle->len + 1;
	}

	printf(": %s\n$:", recvbuff + index);
	fflush(stdout);
}

void parse_flag_11(struct header header, int socketNum)
{
	
	int recv_length;
	uint8_t recv_buff[MAXBUF] = "\0";
	int num_of_handles = 0;
	int i;
	struct handle handle;
	struct header loop_header;

	/* get 11 flag from server */
	if ((recv_length = recv(socketNum, &recv_buff, ntohs(header.len) - sizeof(struct header), 0)) < 0)
	{
        perror("Error in the recv");
        exit(-1);
    }
	memcpy(&num_of_handles, recv_buff, sizeof(int));
	printf("%d clients currently connected\n", num_of_handles);
	printf("$:");

	//for loop recv msgwaitall
	for (i = 0; i < num_of_handles; i++)
	{
		memset(&handle, '\0', sizeof(handle));

		if ((recv_length = recv(socketNum, &loop_header, 3, MSG_WAITALL)) < 0)
		{
			perror("Error in the recv");
			exit(-1);
    	}
		//printf("header len: %d\n", loop_header.len);
		if ((recv_length = recv(socketNum, &handle, ntohs(loop_header.len) - 3, MSG_WAITALL)) < 0)
		{
			perror("Error in the recv");
			exit(-1);
    	}

		print_handle(&handle);
		printf("\n$:");
	}
}

void print_handle(struct handle *handle)
{
	int i;

	for (i = 0; i < handle->len; i++)
	{
		//no null byte or spaces
		if(handle->name[i] != '\0' || handle->name[i] != '\x20')
		{
			printf("%c", handle->name[i]);
		}
	}
}

int readFromStdin(char * buffer)
{
	char aChar = 0;
	int inputLen = 0;        
	buffer[0] = '\0';

	while (inputLen < (MAXBUF - 1) && aChar != '\n')
	{
		aChar = getchar();
		if (aChar != '\n')
		{
			buffer[inputLen] = aChar;
			inputLen++;
		}
	}
	
	// Null terminate the string
	buffer[inputLen] = '\0';
	inputLen++;
	
	return inputLen;
}

void checkArgs(int argc, char * argv[])
{
	/* check command line arguments  */
	if (argc != 4)
	{
		printf("usage: %s handle server-name server-number \n", argv[0]);
		exit(1);
	}
}

void list_packet(int socketNum, char *handle)
{

	int send_length;
	struct header send_header;

	send_header.len = htons(3);
	send_header.flag = FLAG_10;

	//sendBuf(socketNum, header, 3);
	if ((send_length = send(socketNum, &send_header, 3, 0)) < 0)
	{
        perror("error in send");
        exit(-1);
    }
}

void message_packet(int socketNum, int argc, char *argv[])
{
	uint8_t sender_handle_len;
	uint16_t total = 3;
	sender_handle_len = (uint8_t) strlen(argv[1]);
	int sending_handles;
	char *token;
	int ret_val;
	int length;
	char sendBuff[MAXBUF] = {'\0'};
	int i;

	//points to the second string seperated by a space
	token = strtok(NULL, " ");
	if (token == NULL)
	{
		printf("Bad strtok()\n");
	}

	//extract num of dst handles
	ret_val = sscanf(token, "%d", &sending_handles);
	if (ret_val <= 0 || sending_handles < 1 || sending_handles > 9)
	{
		printf("sscanf() failed or too little/many\n");
		return;
	}

	//sender handle len
	sendBuff[total] = sender_handle_len;
	total+= 1;

	//handle name
	memcpy(sendBuff + total, argv[1], sender_handle_len);
	total += sender_handle_len;

	//get sending handle num from before mssg call
	sendBuff[total] = sending_handles;
	total += 1;

	for (i = 0; i < sending_handles; i++)
	{
		token = strtok(NULL, " ");
		if (token == NULL)
		{
			printf("Bad strtok()\n");
		}
		if (token[strlen(token) - 1] == '\n')
		{
			sendBuff[total] = strlen(token) - 1;
			total += 1;
			memcpy(sendBuff + total, token, strlen(token) - 1);
			total += strlen(token) - 1;
		}
		else
		{
			sendBuff[total] = strlen(token);
			total += 1;
			memcpy(sendBuff + total, token, strlen(token));
			total += strlen(token);
		}
	}

	token = strtok(NULL, "\n");

	int count = strlen(token) + 1;
	int new_total = total;
	int data_sent = 0;

	//text message is larger than 200 bytes
	while (count >= 199)
	{
		printf("in loop\n");
		memcpy(sendBuff + total, token + data_sent, 199);
		data_sent = 199 + data_sent;

		total = htons(total + 200);

		//new total pdu len
		memcpy(sendBuff, &total, 2);

		//flag
		sendBuff[2] = FLAG_M;

		count -= 199;

		if ((length = send(socketNum, sendBuff, ntohs(total), 0)) < 0) 
		{
			perror("error in send");
			exit(-1);
		}
		total = new_total;
	}
	// if msg is shorter than 200
	memcpy(sendBuff + total, token + data_sent, count);
	total =  htons(total + count);

	//new total pdu len
	memcpy(sendBuff, &total, 2);

	//flag
	sendBuff[2] = FLAG_M;

	//send the packet out to server
	if ((length = send(socketNum, sendBuff, ntohs(total), 0)) < 0) 
	{
		perror("error in send");
		exit(-1);
	}
	fflush(stdout);
}
	
