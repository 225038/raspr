#include <unistd.h>           ////for write
#include <stdio.h>            ////for printtf
#include <string.h>           ////for malloc
#include "ipc.h"
#include "ipc_structs.h"

#define BUF_SIZE 128

int send(void * self, local_id dst, const Message * msg)
{
    InitInfo *init_info = (InitInfo*)self;


    int return_status = (int) write(init_info->descriptors[init_info->process_id][dst]->write_fd, msg,
            sizeof msg->s_header + msg->s_header.s_payload_len);
    if (return_status < 0)
    {
        perror("Can not write");
        return 1;
    }
    printf("send by %d type %d\n", init_info->descriptors[init_info->process_id][dst]->write_fd, msg->s_header.s_type);
    return 0;
}

int send_multicast(void * self, const Message * msg)
{
    InitInfo *init_info = (InitInfo*)self;
    for (int i = 0; i < init_info->processes_count; ++i)
    {
        if (i == init_info->process_id)
        {
            continue;
        }
        int return_status = send(init_info, i, msg);
        if (return_status < 0)
        {
            perror("Can not send multicast message\n");
            return  -1;
        }
    }
    return 0;
}

int receive(void * self, local_id from, Message * msg)
{

    InitInfo *init_info = (InitInfo*)self;
    local_id id = init_info->process_id;
    printf("Enter process %d receive from %d by %d\n",init_info->process_id, from, init_info->descriptors[id][from]->read_fd);

    int return_status = (int)read(init_info->descriptors[id][from]->read_fd, &msg, sizeof(Message));
    printf("read from %d\n", init_info->descriptors[id][from]->read_fd);

    if (msg->s_header.s_payload_len > 0)
    {
        printf("Enter process %d receive from %d - type: %d\n", id, from, msg->s_header.s_type);
    }

    if (return_status < 0)
    {
        printf("Can not read");
        return -1;
    }
    return 0;
}

int receive_any(void * self, Message * msg) {
    InitInfo *init_info = (InitInfo*)self;

    printf("Enter receive_any %d\n", init_info->process_id);
    for (local_id i = 1; i < init_info->processes_count; ++i) {
        if (i != init_info->process_id) {                            ////not to receive message from itself
            int return_status = receive(init_info, i, msg);
            if (return_status < 0) {
//                printf("Leave <0 receive_any\n");
                return 1;
            }
        }
    }
    printf("Process %d Leave receive_any\n", init_info->process_id);
    return 0;
}
