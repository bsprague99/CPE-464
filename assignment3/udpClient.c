#include "udpClient.h"

//award for worlds smallest main XD
int main (int argc, char *argv[])
 {
	checkArgs(argc, argv);
	processFile(argv);

	return 0;
}

void processFile(char * argv[]){
			
	Connection * server = (Connection *) calloc(1, sizeof(Connection));
	uint32_t clientSeqNum = 0;
	int out_file_fd = 0;
	STATE state = START_STATE;
	
	struct window_struct window;

	while (state != DONE)
	{
		switch (state)
		{
		case START_STATE:
			state = start_state(argv, server, &clientSeqNum);
			setupPollSet();
			addToPollSet(server->sk_num);
			break;
		
		case FILENAME:
			state = filename(argv[1], &out_file_fd, atoi(argv[3]), server);
			break;
			
		case FILE_GOOD:
                state = file_good(&out_file_fd, argv[2]);
                break;

		case RECV_DATA:
                state = receiveData(server, out_file_fd, &window, atoi(argv[4]), &clientSeqNum);
                break;
		case DONE:
                free(window.buffer_size);
				free(server);
                break;
			default:
				state = DONE;
				break;
		}
	}
	return;
}

STATE start_state(char *argv[], Connection *server, uint32_t *clientSeqNum)
{
	int fname_len = strlen(argv[1]);
	STATE returnValue = DONE;
	int sockNum = 0;				
	struct sockaddr_in6 remote;
	uint8_t buff[MAX_LEN];
	memset(buff, '\0', MAX_PDU_LEN);
	uint8_t payload[MAX_PDU_LEN];
	memset(payload, '\0', MAX_PDU_LEN);
	int pdu_len = 0;
	uint32_t window_size;
	uint32_t buffer_size;
	int len;

	//check to see if client has connected before
	if(server->sk_num > 0){
		close(server->sk_num);
	}

	if ((sockNum = setupUdpClientToServer(&remote, argv[6], atoi(argv[7]))) < 0)
	{
		returnValue = DONE;
	}
	else
	{
		server->sk_num = sockNum;
		server->remote = remote;
		server->len = sizeof(struct sockaddr_in6);

		//extract window size from command line argvs
		window_size = htonl(atoi(argv[3]));
		memcpy(payload, &window_size, sizeof(uint32_t));
		pdu_len += sizeof(uint32_t);

		//extract buffer size from command line argvs
		buffer_size = htonl(atoi(argv[4]));
		memcpy(payload + pdu_len, &buffer_size, sizeof(uint32_t));
		pdu_len += sizeof(uint32_t);

		//extract file name from argvs
		memcpy(payload + pdu_len, argv[1], fname_len);
		pdu_len += fname_len;

		//create the pdu to send the argv info to server
		len = createPDU(buff, *clientSeqNum, FLAG_SETUP, payload, pdu_len);
		sendtoErr(server->sk_num, buff, len, 0, (const struct sockaddr *)&(server->remote), server->len);
		(*clientSeqNum)++;

		returnValue = FILENAME;
	}

	return returnValue;
}

STATE filename(char * fname, int *out_file_fd, int32_t buf_size, Connection * server){

	int returnValue = DONE;
	uint8_t buf[MAX_LEN] = {'\0'};
	memset(buf, '\0', MAX_LEN);
	uint8_t *flag = 0;
	uint32_t recv_len = 0;

	if ((recv_len = safeRecvfrom(server->sk_num, buf, MAX_LEN, 0, (struct sockaddr *)&(server->remote), (int *)&(server->len))) < 1)
	{
		returnValue = START_STATE;
	}
	else
	{
		flag = buf + FLAG_OFFSET;

		if (*flag == FNAME_BAD)
		{
			printf("File not found on server...DONE\n");
			returnValue = DONE;
		}
		if (*flag == FNAME_OK)
		{
			returnValue = DONE;

			if((*out_file_fd = open(fname, O_CREAT | O_TRUNC | O_WRONLY, 0600)) < 0)
			{
			perror("File open error: ");
			returnValue = DONE;
			}
			else
			{
				returnValue = FILE_GOOD;
			}
		}
	}
	return returnValue;
}

STATE file_good(int *out_file_fd, char * fname){

	STATE returnValue = DONE;

    if((*out_file_fd = open(fname, O_CREAT | O_TRUNC | O_WRONLY, 0600)) < 0){
        perror("File open error: ");
        returnValue = DONE;
    }
    else{
        returnValue = RECV_DATA;
    }
    return returnValue;

}

STATE receiveData(Connection *server, int32_t outputFile, struct window_struct *window, int buf_size, uint32_t *clientSeqNum)
{
	int32_t len = 0;
	uint8_t buff[MAX_LEN];
	memset(buff, '\0', MAX_LEN);
	uint8_t *flag = 0;
	uint32_t *sequence_Num;
	static uint32_t inOrderSeqNum = 1;
	uint16_t check = 0;
	uint8_t *spot;
	static uint32_t EOFcheck = 10000000;

	if(pollCall(10000) != -1)
	{
		len = safeRecvfrom(server->sk_num, buff, MAX_LEN, 0, (struct sockaddr *)&(server->remote), (int *)&(server->len));
		check = in_cksum((unsigned short *) buff, len);

		if(EOFcheck <= inOrderSeqNum)
		{
			printf("100%% complete....DONE\n");
			sendResponse(server, clientSeqNum, &inOrderSeqNum, 2);
			return DONE;
		}
		if(check != 0)
		{ 
			//checksum detected bad packet
			return RECV_DATA;
		}

		sequence_Num = (uint32_t *)buff;

		*sequence_Num = ntohl(*sequence_Num);

		insertToBuffer(window, buff, len, *sequence_Num);

		flag = buff + FLAG_OFFSET;

		if(*flag == DATA)
		{ 
			//recv normal data pkt
			if(*sequence_Num == inOrderSeqNum)
			{ 
				spot = accessBuffer(window, *sequence_Num);
				
				while((((Packet*)spot)->seqNum) == inOrderSeqNum)
				{
					write(outputFile, spot + HEADER_OFFSET, ((Packet *)spot)->pduSize - HEADER_OFFSET);

					inOrderSeqNum++;

					sendResponse(server, clientSeqNum, &inOrderSeqNum, 0);

					spot = accessBuffer(window, inOrderSeqNum);
				}
			}
			else if(*sequence_Num > inOrderSeqNum) 
			{ 	
				//seq num dont match so send srej
				sendResponse(server, clientSeqNum, &inOrderSeqNum, 1);
			}
			else if(*sequence_Num < inOrderSeqNum)
			{ 
				//no ack recv
				sendResponse(server, clientSeqNum, &inOrderSeqNum, 0);
			}
			return RECV_DATA;
		}
		if(*flag == EOF_F)
		{ 
			EOFcheck = *sequence_Num;		
		}	
		return RECV_DATA;
	}
	return DONE;
}

void sendResponse(Connection *server, uint32_t *clientSeqNum, uint32_t *expSeqNum, int check) 
{
    uint8_t buf[MAX_LEN];
	memset(buf, '\0', MAX_LEN);
    uint8_t pdu[MAX_PDU_LEN];
	memset(pdu, '\0', MAX_PDU_LEN);
	int length = 0;
	uint32_t seq_num;

	//dont forget to network order
	seq_num = htonl(*expSeqNum);
	//insert seq num
    memcpy(pdu, &seq_num, sizeof(uint32_t));

	switch (check)
	{
	case 0:
		length = createPDU(buf, *clientSeqNum, RR, pdu, sizeof(uint32_t));
		break;
	case 1:
		length = createPDU(buf, *clientSeqNum, SREJ, pdu, sizeof(uint32_t));
		break;
	default:
		length = createPDU(buf, *clientSeqNum, EOF_ACK, pdu, sizeof(uint8_t));
		break;
	}
	
    sendtoErr(server->sk_num, buf, length, 0, (const struct sockaddr *)&(server->remote), server->len);
	(*clientSeqNum)++;
}

void checkArgs(int argc, char * argv[])
{
	/* check command line arguments  */
	if (argc != 8)
	{
		printf("usage: %s remote_file local_file WindowSize BufferSize ErrorRate RemoteName RemotePort \n", argv[0]);
		exit(-1);
	}
	/* check remote file valid  */
	if (strlen(argv[1]) > 100)
	{
		printf("Error: remote_file is greater than 1000 characters and is %ld\n", strlen(argv[1]));
		exit(-1);
	}
	/* check local destination valid  */
	if (strlen(argv[2]) > 100)
	{
		printf("Error: local_file is greater than 1000 characters and is %ld\n", strlen(argv[2]));
		exit(-1);
	}
	/* check window size valid  */
	if (atoi(argv[3]) > 1073741824)
	{
		printf("Error: window size needs to be less than 1073741824 and is %d\n", atoi(argv[3]));
		exit(-1);
	}
	/* check buffer size valid  */
	if (atoi(argv[4]) < 1 || atoi(argv[4]) > 1400)
	{
		printf("Error: buffer size needs to be between 1 and 1400 and is %d\n", atoi(argv[4]));
		exit(-1);
	}
	/* check error rate valid  */
	if (atoi(argv[5]) < 0 || atoi(argv[5]) > 1)
	{
		printf("Error: error rate needs to be a decimal between 0 and 1 and is %d\n", atoi(argv[5]));
		exit(-1);
	}
	/* Initilize ErrorToSend and Error Rate */
	double errorRate = atof(argv[5]);
	sendErr_init(errorRate, DROP_ON, FLIP_ON, DEBUG_OFF, RSEED_OFF); 
}

