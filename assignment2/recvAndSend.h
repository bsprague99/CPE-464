#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>

int recvBuf(int clientSocket, uint8_t * dataBuffer, int bufferLen);
int sendBuf(int socketNumber, uint8_t * dataBuffer, int lengthOfData);
