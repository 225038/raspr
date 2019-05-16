#include <unistd.h>           ////for write
#include <stdio.h>            ////for printtf
#include <string.h>           ////for malloc
#include <fcntl.h>
#include "ipc.h"
#include "ipc_structs.h"
#include "ipc_worker.h"

#define BUF_SIZE 128

int send(void * self, local_id dst, const Message * msg)
{
    InitInfo *init_info = (InitInfo*)self;
//    printf("%d To %d\n", init_info->process_id, dst);

    int fd = init_info->descriptors[init_info->process_id][dst]->write_fd;
    int return_status = (int) write(fd, msg, sizeof msg->s_header + msg->s_header.s_payload_len);
    if (return_status < 0)
    {
        perror("Can not write");
        return 1;
    }
//    printf("Leave send\n");
//    printf("process %d send by %d type %d\n", init_info->process_id,
//            init_info->descriptors[init_info->process_id][dst]->write_fd, msg->s_header.s_type);
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
//        printf("send from %d, to %d type %d\n", init_info->process_id, i, msg->s_header->);
        if (return_status < 0)
        {
            perror("Can not send multicast message\n");
            return  -1;
        }
    }
    return 0;
}

int receive(void * self, local_id from, Message* msg)
{

    InitInfo *init_info = (InitInfo*)self;
    local_id id = init_info->process_id;

    int fd = init_info->descriptors[id][from]->read_fd;
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);

//    printf("Enter process %d receive from %d by %d\n",init_info->process_id, from, fd);
    if (read(fd, &msg->s_header, sizeof(MessageHeader)) < 0)
    {
        return -1;
    }
    int return_status = (int)read(fd, msg->s_payload, msg->s_header.s_payload_len);
    if (return_status < 0)
    {
//        printf("Can not read\n");
        return -1;
    }
//    printf("%d %d\n", from, msg->s_header.s_type);
    if (msg->s_header.s_local_time > time) {
        time = msg->s_header.s_local_time;
    }
    time++;
//    printf("RECEIVED by process %d from %d - type: %d, length: %d\n", id, from, msg->s_header.s_type,
//            msg->s_header.s_payload_len);
    return 0;
}

int receive_any(void * self, Message * msg) {

    InitInfo *init_info = (InitInfo*)self;

//    printf("Enter receive_any %d\n", init_info->process_id);
    while (1)
    {
        for (local_id i = 0; i < init_info->processes_count; ++i) {
            if (i != init_info->process_id) {                            ////not to receive message from itself
                int return_status = receive(init_info, i, msg);
                if (return_status == 0) {
//                    if (msg->s_header.s_local_time > time) {
//                        time = msg->s_header.s_local_time;
//                        time++;
//                    }
//                Мы получили сообщение, возвращаемся
                    return i;                                           ////возвращаем индекс, потому что нам надо номер процесса
                }
            }
        }
    }
}
