//lab07
#include "pdu_create.h"

int createPDU(uint8_t * pduBuffer, uint32_t sequenceNum, uint8_t flag, uint8_t * payload, int payloadLen)
{
    int offset_len = 0; 
    uint16_t checkSum = 0; 

    sequenceNum = htonl(sequenceNum);
    memcpy(pduBuffer, &sequenceNum, 4);
    offset_len += 4;

    //this is just putting a dummy value for the checksum
    memcpy(pduBuffer + offset_len, &checkSum, 2);
    offset_len += 2;

    memcpy(pduBuffer + offset_len, &flag, 1);
    offset_len += 1;

    if(payloadLen > 0){
        memcpy(pduBuffer + offset_len, payload, payloadLen);
        offset_len += payloadLen;
    }
    //inserting real checksum after running in_chksum()
    checkSum = in_cksum((short unsigned int *)pduBuffer, offset_len);
    memcpy(pduBuffer + 4, &checkSum, 2);
    return offset_len;
}
