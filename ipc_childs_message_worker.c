#include <stdio.h>              ////for printf
#include <stdlib.h>             ////for malloc
#include <unistd.h>             ////for pipe
#include <string.h>             ////for memset
#include <fcntl.h>

#include "ipc_logs.h"
#include "ipc_childs_message_worker.h"

/**
 * In this function process tells to other that it has started. After that it is waiting for messages from others.
 * @param init_info
 */
void send_start_message_to_parent(InitInfo *init_info)
{
    write_info_to_events_file(log_started_fmt, init_info, STARTED);
    printf(log_started_fmt, get_physical_time(), init_info->process_id, getpid(), getppid(), init_info->bank_account);
    Message msg = generate_message(STARTED, init_info, get_physical_time(), MESSAGE_MAGIC);
    int return_status = send(init_info, 0, &msg);
    if (return_status != 0)
    {
        perror("Can not sent started message to all");
        exit(1);
    }

}

/**
 * In this function process tells to other that it has done. After that it is waiting for messages from others.
 * @param init_info
 */
void send_done_message_to_all(InitInfo *init_info)
{
    write_info_to_events_file(log_done_fmt, init_info, DONE);
    printf(log_done_fmt, get_physical_time(), init_info->process_id, init_info->bank_account);
    Message msg = generate_message(DONE, init_info, get_physical_time(), MESSAGE_MAGIC);
    int return_status = send_multicast(init_info, &msg);
    if (return_status != 0)
    {
        perror("Can not sent done message to all");
        exit(1);                                        //if smth is wrong kill process
    }
    receive_from_every_child(init_info, &msg, DONE);
}

void send_history_message_to_parent(InitInfo *init_info, BalanceHistory balance_history)
{
    Message historyMsg = generate_history_message(balance_history, get_physical_time(), MESSAGE_MAGIC);
//    printf("hist %d balance = %d\n", balance_history.s_id, balance_history.s_history[1].s_balance);
//    send(init_info, 0, &historyMsg);
    send_multicast(init_info, &historyMsg);
    Message msgs[init_info->processes_count - 1];
    receive_from_every_child(init_info, msgs, BALANCE_HISTORY);
}

int receive_from_every_child(void *self, Message *msgs, MessageType type)
{
    InitInfo *init_info = (InitInfo*)self;
//    printf("Enter receive_every %d for %d\n", type, init_info->process_id);
    for (local_id i = 1; i < init_info->processes_count; i++)
    {
        if (i != init_info->process_id) {
//            printf("%d from %d receive type %d\n", init_info->process_id, i, type);
            do
            {
                int fd = init_info->descriptors[init_info->process_id][i]->read_fd;
                fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0));
                read(fd, &msgs[(i-1)], sizeof(Message));
            } while (msgs[(i-1)].s_header.s_type != type);

        }
    }
//    printf("Leave receive_every %d\n", init_info->process_id);
    return 0;
}
