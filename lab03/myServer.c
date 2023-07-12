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

#include "networks.h"
#include "lab3.c"
#include "pollLib.h"

#define MAXBUF 1024
#define DEBUG_FLAG 1

void recvFromClient(int clientSocket);
int checkArgs(int argc, char *argv[]);

int main(int argc, char *argv[])
{
	int serverSocket = 0;   //socket descriptor for the server socket
	int clientSocket = 0;   //socket descriptor for the client socket
	int portNumber = 0;
	int done;
	int status;

	portNumber = checkArgs(argc, argv);
	
	
	//create the server socket
	serverSocket = tcpServerSetup(portNumber);

	done = 0;
	
	setupPollSet();
	addToPollSet(serverSocket);
	do {
		status = pollCall(-1);

		//if poll call returns seversocket, a new client is ready
		if (status == serverSocket) {
			clientSocket = tcpAccept(serverSocket, DEBUG_FLAG);
			addToPollSet(clientSocket);
		}

		//if poll did not timeout, ready to recv
		else if (status != -1) {
			recvFromClient(clientSocket);
		}		
		
	} while(!done);

	/* close the sockets */
	close(clientSocket);
	close(serverSocket);

	
	return 0;
}

void recvFromClient(int clientSocket)
{
	char buf[MAXBUF];
	int messageLen = 0;
	int done;
	char length[47];
	
	done = 0;
	
		//now get the data from the client_socket
		if ((messageLen = recvBuf(clientSocket, (uint8_t *) buf, MAXBUF)) < 0)
		{
			perror("recv call");
			exit(-1);
		}
		else if (messageLen == 0)
		{
			removeFromPollSet(clientSocket);
			return;
		}
		if (!strncmp(buf, "exit", 3)) {
			removeFromPollSet(clientSocket);
			return;
		}
		printf("Message received, length: %d Data: %s\n", messageLen, buf);

		sendBuf(clientSocket, (uint8_t *)buf, messageLen);
		sprintf(length, "Number of bytes received by the server was: %d", messageLen);
		sendBuf(clientSocket, (uint8_t *) length, 47);
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

