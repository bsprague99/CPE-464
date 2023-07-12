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
#include "pollLib.h"

#define MAXBUF 80
#define MAX_LEN 1407
#define MAX_FNAME_LEN 100
#define HEADER_LEN 7

#define DATA 3
#define ACK 5
#define SREJ 6
#define FNAME 7 
#define FNAME_OK 8
#define FNAME_BAD 9
#define EOF_F 10
#define EOF_ACK 11


typedef struct connection Connection;

struct connection
{
	int32_t sk_num; //socket num
	struct sockaddr_in6 remote; //port info
	uint32_t len;
};

typedef enum State STATE;

enum State
{
	START, TIMEOUT,  DONE, FILENAME, RECV_DATA, SEND_DATA, WAIT_EOF_ACK
};


int checkArgs(int argc, char *argv[]);
void processServer(int socketNum);
void processClient(uint32_t serverSockNum, uint8_t *buf, int32_t recv_len, Connection *client);
STATE filename(Connection * client, uint8_t * buf, int32_t recv_len, int * data_file, uint32_t * buf_size, uint32_t * window_size, uint32_t * seqNum, struct window_struct *window);
STATE sendData(Connection *client, int32_t dataFile, uint32_t windowSize, uint32_t buffSize, uint32_t *seqNum, int *sendLen, struct window_struct *window);
STATE waitEOF(Connection *client, struct window_struct *window, uint32_t *seqNum);
STATE recvData(Connection *client, struct window_struct *window, uint32_t *seqNum);
int processPoll(Connection * client, int *retryCount);



/*	Main utilizes udp socket connection abstracted by professor Smith		*/
/*	as well as various states controlled by a state machine to 				*/
/*	achieve a reliable file/data transfer tool that handles many clients	*/
int main ( int argc, char *argv[])
{ 
	int socketNum = 0;				
	int portNumber = 0;

	portNumber = checkArgs(argc, argv);	 //port number assigned from argv here
	socketNum = udpServerSetup(portNumber); //setup server socket for connections
	processServer(socketNum);
	close(socketNum);

	return 0;
}

void processServer(int socketNum)
{
	uint8_t buf[MAX_LEN] = {'\0'};
	int32_t recv_len = 0;
	uint16_t check = 0;

	Connection client;
	client.len = sizeof(struct sockaddr_in6);
	int child_socket;
	pid_t pid = 0;


	while(1)
	{
		if ((recv_len = safeRecvfrom(socketNum, buf, MAX_LEN, 0, (struct sockaddr *)&(client.remote), (int *)&(client.len))) < 0)
		{
			perror("safeRecvFrom()\n");
		}
		if ((check = in_cksum((unsigned short *)buf, recv_len)) == 0)
		{
			if((pid = fork()) == -1)
			{
				perror("fork()");
				exit(-1);
			}
			else if (pid == 0)
			{
				if((child_socket = socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
				{
					perror("bad socket");
					exit(-1);
				}
				client.sk_num = child_socket;
				processClient(child_socket, buf, recv_len, &client);
			}
		}			
	}
}

void processClient(uint32_t serverSockNum, uint8_t *buf, int32_t recv_len, Connection *client){

	STATE state = START;
	int32_t data_file = 0;
	uint32_t buf_size = 0;
	uint32_t window_size = 0;
	uint32_t seqNum = 0;
	struct window_struct window;
	uint8_t * loc;
	int send_len= 0;

	setupPollSet();
	addToPollSet(client->sk_num);

	while (state != DONE)
	{
		switch (state)
		{
		case START:
			state = FILENAME;
			break;

		case FILENAME:
				state = filename(client, buf, recv_len, &data_file, &window_size, &buf_size, &seqNum, &window);
			break;

		case SEND_DATA:
			if(windowstatus(&window) == 0)
			{ //window is closed
				state = RECV_DATA;
			}
			else
			{
				state = sendData(client, data_file, window_size, buf_size, &seqNum, &send_len, &window);
			}
			break;
		case RECV_DATA:
				state = recvData(client, &window, &seqNum);
				break;
		case TIMEOUT:
			state = RECV_DATA;
			loc = pullFromWindow(&window, window.lower);
			sendtoErr(client->sk_num, loc, ((Packet *)loc)->pduSize, 0, (const struct sockaddr *)&(client->remote), client->len);
			seqNum++;
			break;
		case WAIT_EOF_ACK:
			state = waitEOF(client, &window, &seqNum);
			break;
		case DONE:
			free(window.buffer_size);
			free(client);
			return;
		default:
			state = DONE;
			break;
		}
	}
	return;
}

STATE filename(Connection *client, uint8_t *buf, int32_t recv_len, int32_t *data_file, uint32_t *window_size, uint32_t *buf_size, uint32_t * seqNum, struct window_struct *window)
{
	char fname[MAX_FNAME_LEN];
	STATE returnValue = DONE;
	uint8_t send_buf[MAX_LEN] = {'\0'};
	int offset = HEADER_LEN;
	int len;

	memcpy(window_size, buf + offset, sizeof(uint32_t));
	*window_size = ntohl(*window_size);
	offset += sizeof(uint32_t);

	memcpy(buf_size, buf + offset, sizeof(uint32_t));
	*buf_size = ntohl(*buf_size);
	offset += sizeof(uint32_t);

	memcpy(&fname, buf + offset, recv_len - 13);
	//printf("fname: %s\n", fname);

	*data_file = open(fname, O_RDONLY);

	initBuffer(window, *window_size);

	if(*data_file < 0){
		//file not found
		len = createPDU(send_buf, *seqNum, FNAME_BAD, buf, 0);
		sendtoErr(client->sk_num, send_buf, len, 0, (const struct sockaddr *)&(client->remote), client->len);
		(*seqNum)++;
		returnValue = DONE;
	}
	else{
		//file found
		len = createPDU(send_buf, *seqNum, FNAME_OK, buf, 0);
		sendtoErr(client->sk_num, send_buf, len, 0, (const struct sockaddr *)&(client->remote), client->len);
		(*seqNum)++;
		returnValue = SEND_DATA;
	}

	return returnValue;
}

STATE sendData(Connection *client, int32_t dataFile, uint32_t windowSize, uint32_t buffSize, uint32_t *seqNum, int *sendLen, struct window_struct *window)
{
    STATE returnValue = DONE;
	uint8_t packet[MAX_LEN] = {'\0'};
	uint8_t buf[buffSize];
	memset(buf, '\0', buffSize);
	int readLen = 0;
	int packLen = 0;

	readLen = read(dataFile, buf, buffSize);
	
	if (readLen == 0) //EOF
	{
		packLen = createPDU(packet, window->curr, EOF_F, buf, 0);
		(*sendLen) = sendtoErr(client->sk_num, packet, packLen, 0, (const struct sockaddr *)&(client->remote), client->len);
		insertToBuffer(window, packet, packLen, window->curr);
		returnValue = WAIT_EOF_ACK;
	}
	else if (readLen == -1) //failed read
	{
		perror("sendData, read error\n");
		returnValue = DONE;
	}
	else //successful read
	{
		packLen = createPDU(packet, window->curr, DATA, buf, readLen);
		(*sendLen) = sendtoErr(client->sk_num, packet, packLen, 0, (const struct sockaddr *)&(client->remote), client->len);
		insertToBuffer(window, packet, packLen, window->curr);
		(*seqNum)++;
		
		returnValue = RECV_DATA;
	}
    return returnValue;
}

STATE recvData(Connection *client, struct window_struct *window, uint32_t *seqNum)
{
	STATE returnValue = DONE;
	uint8_t buf[MAX_LEN] = {'\0'};
	uint32_t recv_len = 0;
	uint8_t *flag;
	static int retryCount = 0;
	uint8_t *loc;
	int ackNum = 0;
	uint16_t check = 0;
	int check_socket;


    returnValue = SEND_DATA;

	if(windowstatus(window) == 0) //window is closed
	{
		(retryCount)++;
    	if (retryCount > 10)
    	{
        	printf("No Client response for %d seconds, closed\n", 10);
    	}
		else
		{
			if ((check_socket = pollCall(1000)) != -1) //returns -1 if timeout occured
        	{
            	retryCount = 0;
            	recv_len = safeRecvfrom(client->sk_num, buf, MAX_LEN, 0, (struct sockaddr *)&(client->remote), (int *)&(client->len));
				check = in_cksum((unsigned short *) buf, recv_len);

				flag = buf + 6;

				if(check != 0)
				{
					//check error, ignore packet
					return RECV_DATA;
				}
				else if(*flag == ACK)
				{
					//received ACK
					memcpy(&ackNum, buf + 7, sizeof(uint32_t));
					processwindow(window, ntohl(ackNum));
				}
				else if(*flag == SREJ)
				{
					printf("in srej - window closed\n");
					//received SREJ, pull the sequence num from the payload & resend data
					memcpy(&ackNum, buf + 7, sizeof(uint32_t));
					loc = pullFromWindow(window, ntohl(ackNum));
					sendtoErr(client->sk_num, loc, ((Packet *)loc)->pduSize, 0, (const struct sockaddr *)&(client->remote), client->len);
					(*seqNum)++;
					returnValue = RECV_DATA;
				}
				else if(*flag == EOF_ACK)
				{
					returnValue = DONE;
				}
				else
				{
					returnValue = DONE;
				}
        	}
        	else
        	{
            //data not ready
            returnValue = TIMEOUT;
        	}
		}
	}
	else //window is open process the data inside
	{
		returnValue = SEND_DATA;
		while(pollCall(0) != -1)
		{
			recv_len = safeRecvfrom(client->sk_num, buf, MAX_LEN, 0, (struct sockaddr *)&(client->remote), (int *)&(client->len));
			check = in_cksum((unsigned short *) buf, recv_len);
			flag = buf + 6;

			if(check != 0){
				//check error, ignore packet
				return RECV_DATA;
			}
			else if(*flag == ACK){
				//received ACK, update the sequnce num
				memcpy(&ackNum, buf + 7, sizeof(uint32_t));
				slide(window, ntohl(ackNum));
			}
			else if(*flag == SREJ){
				//received SREJ, pull the sequence num from the payload & resend data
				printf("in srej - window open\n");
				memcpy(&ackNum, buf + 7, sizeof(uint32_t));
				loc = pullFromWindow(window, ntohl(ackNum));
				sendtoErr(client->sk_num, loc, ((Packet *)loc)->pduSize, 0, (const struct sockaddr *)&(client->remote), client->len);
				(*seqNum)++;
				returnValue = SEND_DATA;
			}
		}
	}
	return returnValue;
}

STATE waitEOF(Connection *client, struct window_struct *window, uint32_t *seqNum)
{
	printf("in wait EOF\n");
	STATE returnValue;
	uint8_t *loc;

	window->upper = window->curr - 1;
	returnValue = recvData(client, window, seqNum);

	if (returnValue == TIMEOUT)
	{
		loc = pullFromWindow(window, window->lower);
		sendtoErr(client->sk_num, loc, ((Packet *)loc)->pduSize, 0, (const struct sockaddr *)&(client->remote), client->len);
		(*seqNum)++;
	}
	if (returnValue == DONE)
	{
		return returnValue;
	}
	return WAIT_EOF_ACK;
}

int checkArgs(int argc, char *argv[])
{
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

	if (atof(argv[1]) < 0 || atof(argv[1]) > 1)
	{
		perror("Error rate must be between 0 and 1");
		exit(1);
	}
	double errorRate = atof(argv[1]);

	sendErr_init(errorRate, DROP_ON, FLIP_ON, DEBUG_OFF, RSEED_OFF); 
	
	return portNumber;
}


