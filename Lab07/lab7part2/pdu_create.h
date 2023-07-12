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

#define MAXBUF 80

int createPDU(uint8_t * pduBUFFER, uint32_t squenceNum, uint8_t flag, uint8_t * payload, int payLoadLen);
void outputPDU(uint8_t * pduBUFFER, int pduLength);
