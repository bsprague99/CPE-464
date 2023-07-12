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
#include "checksum.h"


#define MAXBUF 80

int createPDU(uint8_t * pduBUFFER, uint32_t squenceNum, uint8_t flag, uint8_t * payload, int payLoadLen){

    uint16_t checksum = 0;


    squenceNum = htonl(squenceNum);
    memcpy(pduBUFFER, &squenceNum, sizeof(uint32_t));
    memcpy(pduBUFFER + 4, &checksum, sizeof(uint16_t));
    memcpy(pduBUFFER + 6, &flag, sizeof(uint8_t));
    memcpy(pduBUFFER + 7, payload, payLoadLen);

    //no magic numbers for 7
    checksum = in_cksum((unsigned short *) pduBUFFER, 7 + payLoadLen);

    memcpy(pduBUFFER + 4, &checksum, sizeof(uint16_t));

    return 7 + payLoadLen;
}

void outputPDU(uint8_t * pduBUFFER, int pduLength){

    uint32_t * sequenceNum;
    uint8_t * flag;
    uint16_t checksum;

    //verify checksum
    checksum = in_cksum((unsigned short *) pduBUFFER, pduLength);
    printf("CheckSum: %hu\n", (unsigned short )checksum);

    //print sequ num
    sequenceNum = (uint32_t *)pduBUFFER;
    printf("SequenceNum: %d\n", ntohl(*sequenceNum));

    //print flag
    flag = pduBUFFER + 6;
    printf("flag: %hhu\n", *flag);

    //print payload
    printf("PayLoad: %s\n", (char *) pduBUFFER + 7);

    //print payload len
    printf("Payload Len: %d\n", pduLength);
}