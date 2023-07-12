#include "window.h"

void init_window(int size){
    int i;
    windowsize = size;
    new_window = (struct window_frame *)sCalloc(size, sizeof(struct window_frame));

    for (i = 0; i < windowsize; i++)
    {
        //set all new window frames to -1
        new_window[i].status = -1;
    }
    current = 0;
    upper = windowsize;
    lower = current;
}

void printwindow(){
    int i;
    printf("Window Size: %d\n", windowsize);
    for(i = 0; i < windowsize - 1; i++)
    {
        if(new_window[i].status == 1)
        {
            //window status is good
            printf("\t%d Sequence Num: %d PDU Size: %d\n", i, new_window[i].sequenceNum, new_window[i].pdu_size);
        }
        else
        {
            //window status bad
            printf("\tBad status\n");
        }
    }
}

void addPDU(char* data, int datalen, uint32_t sequenceNum){
    int i = sequenceNum % windowsize;
    
    if(new_window[i].status == -1)
    { 
        // if -1, then the window spot is free
        new_window[i].status = 1;
        new_window[i].sequenceNum = sequenceNum;
        new_window[i].pdu_size = datalen;
        memcpy(&new_window[i].buffer, data, datalen);
        current += 1;
    }
    else
    {   
        //if !1 then the window spot is open
        printf("This spot in the window is taken\n");
    }
}

struct window_frame getPDU(uint32_t sequenceNum){
    //pmod and get the pdu
    int index = sequenceNum % windowsize;
    return new_window[index];
}

void print_metadata(){
    printf("Server Window - Window Size: %d", windowsize);
    printf("Lower: %d", lower);
    printf("Upper: %d", upper);
    printf("Current: %d", current);
    printf("Window Status: %d\n", isWindowopen());
}

void processRR(int RR){
    int i;
    if (RR > (current))
    {
        printf("too high of an RR\n");
        return;
    }

    //shift upper and lower
    if (RR > pastRR)
    {
        upper += (RR - pastRR);
        lower += (RR - pastRR);
        printf("processing RR %d...\n", RR);
    

        //clear any entries below the cur RR
        for(i = 0; i < windowsize; i++)
        {
            if((new_window[i].status) && (new_window[i].sequenceNum < RR))
            {
                new_window[i].status = -1;
            }
        }
        pastRR = RR;
    }
}

int isWindowopen(){
    if (current < upper)
    {
        return 1;
    }
    return 0;
}
