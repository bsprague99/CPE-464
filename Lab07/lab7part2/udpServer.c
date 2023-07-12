/* Server side - UDP Code			    */
/* By Hugh Smith	4/1/2017	*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "gethostbyname.h"
#include "networks.h"
#include "safeUtil.h"
#include "cpe464.h"
#include "pdu_create.h"
#include "window.h"


#define MAXBUF 80

void processClient(int socketNum);
int checkArgs(int argc, char *argv[]);

int main ( int argc, char *argv[]  )
{ 
	int socketNum = 0;				
	int portNumber = 0;

	if (atof(argv[1]) < 0 || atof(argv[1]) > 1)
	{
		perror("Error rate must be between 0 and 1");
		exit(1);
	}
	double errorRate = atof(argv[1]);
	sendErr_init(errorRate, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF); 
	
	portNumber = checkArgs(argc, argv);
		
	socketNum = udpServerSetup(portNumber);

	processClient(socketNum);

	close(socketNum);
	
	return 0;
}

void processClient(int socketNum)
{
	int dataLen = 0; 
	char buffer[MAXBUF + 1];	  
	struct sockaddr_in6 client;		
	int clientAddrLen = sizeof(client);	
	buffer[0] = '\0';

	int sequenceNum = 0;
	uint8_t pdubuf[100];
	int PDUlen = 0;
	init_window(3);
	while(1)
	{
		if(isWindowopen() == 1)
		{
			
			PDUlen = createPDU(pdubuf, sequenceNum, 1, (uint8_t*)buffer, 20 + sequenceNum);
			print_metadata();
			addPDU((char*)pdubuf, 20 + sequenceNum, sequenceNum);
			sequenceNum =sequenceNum + 1;
		}
		else
		{
			printwindow();
			printmeta();
			int RR;
			printf("Enter Number to RR: ");
			scanf("%d", &RR);
			processRR(RR);
			printwindow();
			printmeta();
		}
	}


	while (buffer[0] != '.')
	{
		dataLen = safeRecvfrom(socketNum, buffer, MAXBUF, 0, (struct sockaddr *) &client, &clientAddrLen);
	
		printf("Received message from client with ");
		printIPInfo(&client);
		printf(" Len: %d \'%s\'\n", dataLen, buffer);

		// just for fun send back to client number of bytes received
		sprintf(buffer, "bytes: %d", dataLen);
		sendtoErr(socketNum, buffer, strlen(buffer)+1, 0, (struct sockaddr *) & client, clientAddrLen);

	}
}

int checkArgs(int argc, char *argv[])
{
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 3)
	{
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(-1);
	}
	
	if (argc == 3)
	{
		portNumber = atoi(argv[2]);
	}
	
	return portNumber;
}


