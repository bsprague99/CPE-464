/******************************************************************************
* myClient.c
*
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

#include "networks.h"
#include "lab3.c"

#define MAXBUF 1024
#define DEBUG_FLAG 1

void sendToServer(int socketNum);
int readFromStdin(char * buffer);
void checkArgs(int argc, char * argv[]);

int main(int argc, char * argv[])
{
	int socketNum = 0;         //socket descriptor

	checkArgs(argc, argv);

	/* set up the TCP Client socket  */
	socketNum = tcpClientSetup(argv[1], argv[2], DEBUG_FLAG);

	sendToServer(socketNum);
	
	close(socketNum);
	
	return 0;
}

void sendToServer(int socketNum)
{
	char sendBuff[MAXBUF];   //data buffer
	int sendLen = 0;        //amount of data to send
	int sent = 0;			//actual amount of data sent/* get the data and send it   */
	int done;
	int messageLen = 0;
	int messageLen_2 = 0;
	char recvBuff[MAXBUF];
	char recvBuff_2[MAXBUF];
         
	done = 0;

	do {

		sendLen = readFromStdin(sendBuff);
		printf("read: %s string len: %d (including null)\n", sendBuff, sendLen);
		
		sent =  sendBuf(socketNum, (uint8_t *) sendBuff, sendLen);
		if (sent < 0)
		{
			perror("send call");
			exit(-1);
		}

		if (!strncmp(sendBuff, "exit", 3)) 
		{
			break;
		}

		printf("Amount of data sent is: %d\n", sent);
		messageLen = 0;


		//Here is the client getting a confirmation back from server containing the OG message
		if ((messageLen = recvBuf(socketNum, (uint8_t *)recvBuff, MAXBUF)) < 0)
		{
			perror("recv call");
			exit(-1);
		}
		else if (messageLen == 0)
		{
			break;
		}
		printf("Recv() from Server:  %s\n", recvBuff);

		//Here is the client getting confirmation of numOfBytes recvieved
		if ((messageLen_2 = recvBuf(socketNum, (uint8_t *)recvBuff_2, MAXBUF)) < 0)
		{
			perror("recv call");
			exit(-1);
		}
		else if (messageLen_2 == 0)
		{
			break;
		}
		printf("Recv() from Server: %s\n", recvBuff_2);


	} while(!done);
}

int readFromStdin(char * buffer)
{
	char aChar = 0;
	int inputLen = 0;        
	
	// Important you don't input more characters than you have space 
	buffer[0] = '\0';
	printf("Enter data: ");
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
	if (argc != 3)
	{
		printf("usage: %s host-name port-number \n", argv[0]);
		exit(1);
	}
}
