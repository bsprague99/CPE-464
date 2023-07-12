#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "safeUtil.h"

struct window_frame {
    uint8_t buffer[1407];
    int pdu_size;
    int status;
    uint32_t sequenceNum;
};

static struct window_frame* new_window;
static int windowsize;
static int upper;
static int lower;
static int current;
static int pastRR = 0;

void init_window(int size);
void printmeta();
void addPDU(char* data, int datalen, uint32_t sequenceNum);
struct window_frame getPDU(uint32_t sequenceNum);
void print_metadata();
void processRR(int RR);
int isWindowopen();
