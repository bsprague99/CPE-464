#include "handle_table.h"
#include <stdint.h>

void add_item(handle_list *list, int socket_num, handle *new_handle, int *num_items, int *MAX_HANDLES)
{
    int i;
    handle_list *curr;

    if ( *num_items > *MAX_HANDLES )
    {
        *MAX_HANDLES = *MAX_HANDLES * 2;
        list = (handle_list *) realloc(list, *MAX_HANDLES * sizeof(handle_list));
    }
    for (i = 0; i < *MAX_HANDLES; i++)
    {
        curr = &list[i];
        if (curr->name_len == 0) {
            curr->socket_num = socket_num;
            curr->name_len = new_handle->len;
            memcpy(curr->handle_name, new_handle->name, curr->name_len);
            *num_items += 1;
            return;
        }
    }
}

void print_table(handle_list *list, int *num_items, int *MAX_HANDLES)
{
    handle_list *curr;
    int i;

    for (i = 0; i < *MAX_HANDLES; i++)
    {
        curr = &list[i];
        if (curr->name_len != 0) 
        {
            //*num_items += 1; ???
        }
   }
}

int check_table(handle_list *list, handle *new_handle, int *num_items)
{
    handle_list* cur;
    int i;

    for (i = 0; i < *num_items; i++)
    {
        
        cur = &list[i];

        if (0 == compare_handles(cur, new_handle))
        {
            // handle exists
            return i;
        }
    }
    //handle does not exist
    return -1;
}

int compare_handles(handle_list *list, handle *new_handle){
    int i;
  
    if (list->name_len != new_handle->len)
    {
        // these handles are not the same
        return 1;
    }
    else
    {
        for (i = 0; i < new_handle->len; i++)
        {
            if (list->handle_name[i] != new_handle->name[i])
            {
                // these handles are not the same
                return 1;
            }
        }
    }
    return 0;
}

handle_list* create_handle_list(int size) 
{
    return (handle_list *)calloc(size, sizeof(handle_list));
}

void free_handle_list(handle_list *list)
{
    free(list);
}
