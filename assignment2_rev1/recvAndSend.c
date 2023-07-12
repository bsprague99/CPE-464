#include "recvAndSend.h"

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
