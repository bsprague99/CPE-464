/******************************************************************************
* tcp_server.c
*
* CPE 464 - Program 1
*****************************************************************************/

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
#define L_RESPONSE_FLAG 11

void recvFromClient(int clientSocket, struct handle_list* list, int *num_items, int *max_handles);
int checkArgs(int argc, char *argv[]);
void incoming_sockets(int serverSocket, struct handle_list* list, int *num_items, int *max_handles);
void print_handle(struct handle *handle);
int check_table(handle_list *list, handle *new_handle, int *num_items);
void print_table(handle_list *list, int *num_items, int *max_handles);
void remove_from_table(handle_list *list, int socketNum, int *max_handles);
void parse_chat_header(struct header header, int clientSocket, struct handle_list* list, int *num_items, int *max_handles);
void parse_flag_5(int clientSocket, struct header header, struct handle_list* list, int *num_items, int *max_handles);
void parse_flag_10(struct header header, int *num_items, int *max_handles, struct handle_list* list, int clientSocket);
void parse_flag_8(int clientSocket, struct header header, struct handle_list *list, int *num_items, int *max_handles);
void parse_flag_4(int clientSocket, struct header header, struct handle_list *list, int *num_items, int *max_handles);

int main(int argc, char *argv[])
{
	int serverSocket = 0;   //socket descriptor for the server socket
	int clientSocket = 0;   //socket descriptor for the client socket
	int portNumber = 0;
	handle_list *list;
	int num_items = 0;
	int max_handles = 350;

	portNumber = checkArgs(argc, argv);
	
	list = create_handle_list(max_handles);

	//create the server socket
	serverSocket = tcpServerSetup(portNumber);
	
	setupPollSet();
	addToPollSet(serverSocket);
	
	
	incoming_sockets(serverSocket, list, &num_items, &max_handles);

	/* close the sockets */
	close(clientSocket);
	close(serverSocket);
	
	/* free the handle list */
	free_handle_list(list);

	return 0;
}

void incoming_sockets(int serverSocket, struct handle_list* list, int *num_items, int *max_handles)
{
	int clientSocket = 0;   //socket descriptor for the client socket
	int status;
	int length;
	struct header header;
	struct handle handle;

	while (1)
	{
		status = pollCall(-1);

		// New Client
		if (status == serverSocket) 
		{
			clientSocket = tcpAccept(serverSocket, DEBUG_FLAG);
			if ((length = recv(clientSocket, &header, sizeof(struct header), MSG_WAITALL)) < 0)
			{
				perror("Error in payload");
				// return -1;
				exit(-1);
    		}
			
			if (length == 0)
			{
				printf("Client socket has terminated\n");
				removeFromPollSet(clientSocket);
				continue;
			}
			if ((length = recv(clientSocket, &handle, ntohs(header.len) - 3, MSG_WAITALL)) < 0)
			{
				perror("Error in payload");
				// return -1;
				exit(-1);
    		}
		
			if (0 < check_table(list, &handle, num_items))
			{
				//handle already exists
				header.flag = 3;
				printf("Client socket has terminated\n");
				if ((length = send(clientSocket, &header, ntohs(header.len), 0)) < 0) 
				{
					perror("error in send");
					// return -1;
					exit(-1);
				}
				removeFromPollSet(clientSocket);
			}
			if (0 > check_table(list, &handle, num_items))
			{
				addToPollSet(clientSocket);
				add_item(list, clientSocket, &handle, num_items, max_handles);
				print_table(list, num_items, max_handles);
				header.flag = 2;

				if ((length = send(clientSocket, &header, ntohs(header.len), 0)) < 0) 
				{
					perror("error in send");
					exit(-1);
				}
			}
		}
		//recv from already added clients
		else if (status != -1)
		{
			recvFromClient(status, list, num_items, max_handles);
		}
	}
}

void print_handle(struct handle *handle)
{
	int i;

	for (i = 0; i < handle->len; i++)
	{
		printf("%c", handle->name[i]);
	}
	printf("\n");
}

void recvFromClient(int clientSocket, struct handle_list* list, int *num_items, int *max_handles)
{
	struct header header;
	int messageLen = 0;
	
	//now get the data from the client_socket
	if ((messageLen = recv(clientSocket, &header, sizeof(struct header), MSG_WAITALL)) < 0)
	{
		perror("recv call");
		exit(-1);
	}
	if (messageLen == 0)
	{
		//if recv a len of 0 then client has closed, remove from poll set and table
		//remove from table set name len = 0 and sock to -1
		close(clientSocket);
		removeFromPollSet(clientSocket);
		remove_from_table(list, clientSocket, num_items);
		return;
	}
	parse_chat_header(header, clientSocket, list, num_items, max_handles);
}

void parse_chat_header(struct header header, int clientSocket, struct handle_list* list, int *num_items, int *max_handles)
{
	uint8_t flag = header.flag;

	switch(flag){
      	case 2:
         	break;
		case 4:
			parse_flag_4(clientSocket, header, list, num_items, max_handles);
			break;
      	case 5:
         	parse_flag_5(clientSocket, header, list, num_items, max_handles);
		 	break;
		case 8:
         	parse_flag_8(clientSocket, header, list, num_items, max_handles);
		 	break;
      	case 10:
         	parse_flag_10(header,num_items, max_handles, list, clientSocket);
         	break;
	}
}

void parse_flag_4(int clientSocket, struct header header, struct handle_list *list, int *num_items, int *max_handles)
{
	char recv_buf[MAXBUF] = {'\0'};
	char send_buf[MAXBUF] = {'\0'};
	handle_list* cur;
	struct handle *handle;
	int i, len1, x;
	
	if ((len1 = recv(clientSocket, recv_buf, ntohs(header.len) - sizeof(struct header), MSG_WAITALL)) < 0)
	{
		perror("recv call");
		exit(-1);
	}

	handle = (struct handle *)recv_buf;

	for (i = 0; i < *max_handles; i++)
	{
		cur = &list[i];
		x = compare_handles(cur, handle);

		if (cur->socket_num > 1)
        {	
			if (x != 0)
			{
				memcpy(send_buf, &header, 3);
				memcpy(send_buf + 3, &recv_buf, htons(header.len));
				send(cur->socket_num, send_buf, ntohs(header.len), 0);
			}
        }
	}
}

void parse_flag_5(int clientSocket, struct header header, struct handle_list* list, int *num_items, int *max_handles)
{
	char bad_handle_buf[MAXBUF] = {'\0'};
	char sendBuff[MAXBUF] = {'\0'};
	char buf[MAXBUF] = {'\0'};
	char number_of_handles = 0;
	struct header bad_header;
	struct handle *handle;
	int i, socketNum, x, len, pdu_len, len1 = 0, index, messageLen = 0;
	
	if ((len1 = recv(clientSocket, buf, ntohs(header.len) - sizeof(struct header), MSG_WAITALL)) < 0)
	{
		perror("recv call");
		exit(-1);
	}

	handle = (struct handle *)buf;
	index = handle->len + 1;
	
	number_of_handles = buf[index];

	index += 1;
	fflush(stdout);

	//print_table(list, num_items, max_handles);
	for (i = 0; i < number_of_handles; i++)
	{
		handle = (struct handle *)(buf + index);		
		x = check_table(list, handle, num_items);

		index += handle->len + 1;
		fflush(stdout);
		if (-1 != x)
		{
			socketNum = list[x].socket_num;
			memcpy(sendBuff, &header, sizeof(header));
			memcpy(sendBuff + sizeof(header), buf, len1);

			if ((len = send(socketNum, sendBuff, ntohs(header.len), 0)) < 0) 
			{
				perror("error in send");
				// return -1;
				exit(-1);
			}
		}
		else
		{
			bad_header = header;
			bad_header.flag = 7;

			len = strlen(list[x].handle_name);
			
			pdu_len = len + sizeof(struct header) + 1;
			header.len =  pdu_len;
			
			memcpy(bad_handle_buf, &bad_header, sizeof(struct header));
			memcpy(bad_handle_buf + sizeof(struct header), &handle, sizeof(struct handle));

			if ((len = send(list[x].socket_num, &bad_header, messageLen, 0)) < 0) 
			{
				perror("error in send");
				// return -1;
				exit(-1);
			}
		}
	}
}

void parse_flag_8(int clientSocket, struct header header, struct handle_list *list, int *num_items, int *max_handles)
{
	int send_len = 0;
	struct header response;

	response.flag = 9;
	response.len =  htons(3);

	if ((send_len = send(clientSocket, &response, ntohs(response.len), 0)) < 0)
	{
        perror("error in send");
        exit(-1);
    }

	remove_from_table(list, clientSocket, num_items);
	removeFromPollSet(clientSocket);
	close(clientSocket);
}

void parse_flag_10(struct header header, int *num_items, int *max_handles, struct handle_list* list, int clientSocket)
{
	uint8_t L_send_buff[MAXBUF];
	struct header L_header;
	int L_send_len = 0;
	uint8_t buffer[MAXBUF];
	int i;
	handle_list* cur;
	struct header loop_header;
	
	L_header.flag = 11;
	L_header.len = htons(7);

	memcpy(L_send_buff, &L_header, 3);
	memcpy(L_send_buff + 3, num_items, sizeof(int));

	if ((L_send_len = send(clientSocket, L_send_buff, ntohs(L_header.len), 0)) < 0)
	{
		perror("error in send");
		exit(-1);
	}

	loop_header.flag = 12;
	for (i = 0; i < *max_handles; i++)
	{
		cur = &list[i];
		if (cur->socket_num > 1)
        {
			loop_header.len = htons(4 + cur->name_len);

			memcpy(buffer, &loop_header, 3);
			memcpy(buffer + 3, &cur->name_len, 1);
			memcpy(buffer + 4, &cur->handle_name, cur->name_len);

			send(clientSocket, buffer, ntohs(loop_header.len), 0);
        }
	}
}

void remove_from_table(handle_list *list, int socketNum, int *num_items)
{
    handle_list* cur;
    int i;
	
    for (i = 0; i < *num_items; i++)
    {
        cur = &list[i];

        if (cur->socket_num == socketNum)
        {
			cur->name_len = 0;
            cur->socket_num = -1;
			*num_items -= 1; //remove from total set
        }
    }
}

int checkArgs(int argc, char *argv[])
{
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 2)
	{
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(-1);
	}
	if (argc == 2)
	{
		portNumber = atoi(argv[1]);
	}
	return portNumber;
}

