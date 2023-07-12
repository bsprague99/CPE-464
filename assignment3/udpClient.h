// Client side - UDP Code				    
// By Hugh Smith	4/1/2017		

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

#include "gethostbyname.h"
#include "networks.h"
#include "safeUtil.h"
#include "pdu_create.h"
#include "cpe464.h"
#include "window.h"
#include "pollLib.h"
#include "window.h"

#define SIZE_OF_BUF_SIZE 4
#define MAX_LEN 1407
#define MAXBUF 80
#define MAX_PDU_LEN 1400
#define FLAG_OFFSET 6
#define HEADER_OFFSET 7

#define DATA 3
#define RR 5
#define SREJ 6
#define FLAG_SETUP 7
#define FNAME_OK (uint8_t) 8
#define FNAME_BAD (uint8_t) 9
#define EOF_F 10
#define EOF_ACK 11

typedef struct connection Connection;

struct connection
{
	int32_t sk_num;
	struct sockaddr_in6 remote;
	uint32_t len;
};

typedef enum State STATE;

enum State
{
	DONE, FILENAME, RECV_DATA, FILE_GOOD, START_STATE
};

void talkToServer(int socketNum, struct sockaddr_in6 * server);
int readFromStdin(char * buffer);
void checkArgs(int argc, char * argv[]);
void processFile(char * argv[]);
STATE file_good(int *out_file_fd, char * fname);
void sendResponse(Connection *server, uint32_t *clientSeqNum, uint32_t *expSeqNum, int check);
STATE start_state(char * argv[], Connection * server, uint32_t * clientSeqNum);
STATE filename(char * fname, int *out_file_fd, int32_t buf_size, Connection * server);
STATE receiveData(Connection *server, int32_t outputFile, struct window_struct *window, int buf_size, uint32_t *clientSeqNum);
STATE file_ok(int *out_file_fd, char * fname);
