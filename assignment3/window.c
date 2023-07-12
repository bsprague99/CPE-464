#include "window.h"

void initBuffer(struct window_struct *window, uint32_t size) {
    window->buffer_size = malloc(size * sizeof(Packet));
    window->size = size;
    window->upper = size;
    window->curr = 1;
    window->lower = 1;
}
//add to window buffer at a location inside using %
void insertToBuffer(struct window_struct *window, uint8_t *data, int dataLen, uint32_t sequenceNum){
    uint8_t *spot;

    spot = window->buffer_size + ((sequenceNum % window->size) * sizeof(Packet));

    memcpy(spot, data, dataLen);

    window->curr++;

    //segfaults here sometimes if you try the first rcopy too soon after starting server
    //printf("here1\n");
    memcpy(spot + sizeof(Packet) - sizeof(uint32_t), &dataLen, sizeof(uint32_t));
    //printf("here2\n");

    return;
}


uint8_t *accessBuffer(struct window_struct *window, uint32_t sequenceNum){
    uint8_t *loc;
    loc = window->buffer_size + ((sequenceNum % window->size) * sizeof(Packet));
    return loc;
}

//helper function that returns window's open & close status
int windowstatus(struct window_struct *window){
    if (window->curr <= window->upper)
    {
        //open
        return 1;
    }
    //closed
    return 0;
}






































// #include "window.h"

// static int pastRR = 0;

// struct window_frame* init_window(struct window_frame *new_window, uint32_t size){
//     int i;
//     new_window = (struct window_frame *)sCalloc(size, sizeof(struct window_frame));

//     for (i = 0; i < size; i++)
//     {
//         //set all new window frames to -1
//         new_window[i].status = -1;
//     }
//     new_window->window_size = size;
//     new_window->upper = size + 1;
//     new_window->current = 1;
//     new_window->lower = 1;

//     return new_window;
// }

// void printwindow(){
//     int i;
//     printf("Window Size: %d\n", new_window->window_size);
//     for(i = 0; i < new_window->window_size; i++)
//     {
//         if(new_window[i].status == 1)
//         {
//             //window status is good
//             printf("\t%d Sequence Num: %d PDU Size: %d\n", i, new_window[i].sequenceNum, new_window[i].pdu_size);
//         }
//         else
//         {
//             //window status bad
//             printf("\tBad status\n");
//         }
//     }
// }

// void addPDU(char* data, int datalen, uint32_t sequenceNum){
//     int i = sequenceNum % new_window->window_size;
    
//     if(new_window[i].status == -1)
//     { 
//         // if -1, then the window spot is free
//         new_window[i].status = 1;
//         new_window[i].sequenceNum = sequenceNum;
//         new_window[i].pdu_size = datalen;
//         memcpy(&new_window[i].buffer, data, datalen);
//         new_window->current += 1;
//     }
//     else
//     {   
//         //if !1 then the window spot is open
//         printf("This spot in the window is taken\n");
//     }
// }

// struct window_frame getPDU(uint32_t sequenceNum){
//     //pmod and get the pdu
//     int index = sequenceNum % new_window->window_size;
//     return new_window[index];
// }

// void print_metadata(){
//     printf("Server Window - Window Size: %d ", new_window->window_size);
//     printf("Lower: %d ", new_window->lower);
//     printf("Upper: %d ", new_window->upper);
//     printf("Current: %d ", new_window->current);
//     printf("Window Status: %d\n", isWindowopen());
// }

// void processRR(int RR){
//     int i;
//     if (RR > (new_window->current))
//     {
//         printf("too high of an RR\n");
//         return;
//     }

//     //shift upper and lower
//     if (RR > pastRR)
//     {
//         new_window->upper += (RR - pastRR);
//         new_window->lower += (RR - pastRR);
//         printf("processing RR %d...\n", RR);
    

//         //clear any entries below the cur RR
//         for(i = 0; i < new_window->window_size; i++)
//         {
//             if((new_window[i].status) && (new_window[i].sequenceNum < RR))
//             {
//                 new_window[i].status = -1;
//             }
//         }
//         pastRR = RR;
//     }
// }

// int isWindowopen(){
//     if (new_window->current < new_window->upper)
//     {
//         return 1;
//     }
//     return 0;
// }
