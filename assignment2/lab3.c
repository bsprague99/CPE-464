#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

int recvBuf(int clientSocket, uint8_t * dataBuffer, int bufferLen);
int sendBuf(int socketNumber, uint8_t * dataBuffer, int lengthOfData);

int recvBuf(int clientSocket, uint8_t * dataBuffer, int bufferLen){
    uint16_t bytesReceived;
    uint16_t PDU_len;

    if ((bytesReceived = recv(clientSocket, &PDU_len, sizeof(uint8_t) * 2, MSG_WAITALL)) < 0){
        perror("Error in the recv");
        exit(-1);
    }
    else if (bytesReceived == 0){ //user wants too exit via a keyboard interupt
        return 0;
    }

    PDU_len = ntohs(PDU_len);

    // printf("Received PDU Length is: %d\n", PDU_len);

    if (PDU_len - 2 > bufferLen){
        perror("Buffer overflow");
        // return -1;
        exit(-1);
    }
    if ((bytesReceived = recv(clientSocket, dataBuffer, PDU_len - 2, MSG_WAITALL)) < 0){
        perror("Error in payload");
        // return -1;
        exit(-1);
    }
    return bytesReceived;
}

int sendBuf(int socketNumber, uint8_t * dataBuffer, int lengthOfData){
    uint16_t bytesSent;
    uint8_t PDU_Buffer[lengthOfData + 2];
    uint16_t PDU_size = htons(lengthOfData + 2);

    memcpy(PDU_Buffer, &PDU_size, sizeof(uint8_t) * 2);
    memcpy(PDU_Buffer + 2, dataBuffer, lengthOfData);

    if ((bytesSent = send(socketNumber, PDU_Buffer, lengthOfData + 2, 0)) < 0) {
        perror("error in send");
        // return -1;
        exit(-1);
    }
    else if (bytesSent == 0){
        perror("connection closed in send");
        // return -1;
        exit(-1);
    }
    return bytesSent;
}


// int recvBuf(int clientSocket, uint8_t *dataBuffer, int bufferLen) {
//     uint16_t PDU_LEN;
//     uint16_t NUM_RECV_BYTES;
    
//     if (recv(clientSocket, &PDU_LEN, 2, MSG_WAITALL) < 0){ //if 1st recv is 0 then dont call second recv, just return 0. Means otherside closed connection
//         perror("recv");
//     }
//     PDU_LEN = ntohs(PDU_LEN);

//     if (PDU_LEN - 2 > bufferLen){
//         printf("data buff size\n");
//         exit(1);
//     }

//     if ((NUM_RECV_BYTES = recv(clientSocket, dataBuffer, PDU_LEN - 2, MSG_WAITALL) < 0)){
//         perror("recvieve");
//     }
//     return NUM_RECV_BYTES;
// }

// int sendBuf(uint8_t socketNumber, uint8_t *dataBuffer, int lengthOfData){
//     uint16_t num_bytes;
//     uint16_t PDU_LEN = lengthOfData + 2;
//     PDU_LEN = htons(PDU_LEN);
//     uint8_t PDUbuffer[1500];

//     memcpy(PDUbuffer, &PDU_LEN, 2);
//     memcpy(PDUbuffer + 2, dataBuffer, lengthOfData);

//     num_bytes = send(socketNumber, PDUbuffer, lengthOfData + 2, 0);

//     return num_bytes;
// }