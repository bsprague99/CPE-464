#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "safeUtil.h"

#include "gethostbyname.h"
#include "safeUtil.h"
#include "networks.h"

#include "cpe464.h"
#include "pdu_create.h"

#define MAX_PDU_LEN 1400

struct window_struct
{
    uint32_t size;
    uint32_t upper;
    uint32_t curr;
    uint32_t lower;
    uint8_t *buffer_size;
};

//attribute packed to that the extra fluff is taken care of
typedef struct Packet 
{
    uint32_t seqNum;
    uint16_t check;
    uint8_t flag;
    uint8_t pdu[MAX_PDU_LEN];
    uint32_t pduSize;
}__attribute__((packed))Packet;

void initBuffer(struct window_struct *window, uint32_t size);
void insertToBuffer(struct window_struct *window, uint8_t *data, int dataLen, uint32_t sequenceNum);
uint8_t *accessBuffer(struct window_struct *window, uint32_t sequenceNum);
int windowstatus(struct window_struct *window);
