#include <stdio.h>  
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


typedef struct handle
{
   char len;
   char name[100];
}__attribute__((packed)) handle;

typedef struct header
{
   uint16_t len;
   uint8_t flag;
}__attribute__((packed)) header;

//handle node
typedef struct handle_list{
    int socket_num;
    char name_len;
    char handle_name[100];
    
} handle_list;

void add_item(handle_list *list, int socket_num, handle *new_handle, int *num_items, int *MAX_HANDLES);
void print_table(handle_list *list, int *num_items, int *MAX_HANDLES);
int check_table(handle_list *list, handle *new_handle, int *num_items);
int compare_handles(handle_list *list, handle *new_handle);
handle_list* create_handle_list(int size);
void free_handle_list(handle_list *list);
