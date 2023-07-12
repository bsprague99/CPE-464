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
