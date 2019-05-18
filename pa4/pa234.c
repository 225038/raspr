#include <getopt.h>         ////for getopt
#include <stdlib.h>         ////for atoi
#include <stdio.h>          ////for printf

#include "ipc_worker.h"

timestamp_t queue[MAX_PROCESS_ID];
int mutexl = 0;
//Parse argv and get param of -p key; +1 in return because general number of processes includes parent process.
int get_processes_count(int argc, char* argv[])
{
    int output = 0;             //return number of processes
    getopt(argc, argv, "p:");
    output = atoi(optarg);
    return output + 1;
}

void transfer(void * parent_data, local_id src, local_id dst, balance_t amount)
{
//    printf("Enter transfer\n");
    Message msg;

    TransferOrder order;
    order.s_src = src;
    order.s_dst = dst;
    order.s_amount = amount;

    msg = generate_transfer_message(order, get_lamport_time(), MESSAGE_MAGIC);
    send(parent_data, src, &msg);
    write_transfer_info_to_events_file(log_transfer_out_fmt, &order, TRANSFER);
    printf(log_transfer_out_fmt, get_lamport_time(), src, amount, dst);
    fflush(stdout);

    while (1)
    {
//        printf("type %d\n", msg.s_header.s_type);
        receive_any(parent_data, &msg);
        if (msg.s_header.s_type == ACK){
            goto leave_transfer;
        }
    }
    leave_transfer:
    write_transfer_info_to_events_file(log_transfer_in_fmt, &order, DONE);
    printf(log_transfer_in_fmt, get_lamport_time(), dst, amount, src);
    fflush(stdout);
//    printf("Leave transfer\n");
}

int who_is_next(const InitInfo *init_info)
{

    for (int i = 1; i < init_info->processes_count; ++i)
    {
//        printf("i = %d, id = %d, queue[i] = %d\n", i, init_info->process_id, queue[i]);
//        fflush(stdout);
        if (i != init_info->process_id && queue[i] >= 0)
        {
//            printf("process %d with %d "
//                   "VS process %d with %d\n", init_info->process_id, queue[init_info->process_id], i, queue[i]);
//            fflush(stdout);
            if (queue[init_info->process_id] > queue[i]){
//                printf("if) process %d goes to the cs\n", i);
//                fflush(stdout);
                return i;
            }
            if (queue[i] == queue[init_info->process_id] && i < init_info->process_id)
            {
//                printf("if2) process %d goes to the cs\n", i);
//                fflush(stdout);
                return i;
            }
        }
    }

//    printf("process %d goes to the cs\n", init_info->process_id);
//    fflush(stdout);
    return 0;
}

int request_cs(const void * self)
{
    InitInfo *init_info = (InitInfo*)self;
    time++;
//    printf("generate %d with %d\n", init_info->process_id, get_lamport_time());
//    fflush(stdout);
    Message msg = generate_empty_message(get_lamport_time(), MESSAGE_MAGIC, CS_REQUEST);
    queue[init_info->process_id] = msg.s_header.s_local_time;
//    printf("%d\n", init_info->process_id);
//    fflush(stdout);
    send_multicast(init_info, &msg);            ////отправили всем, что хотим в КС

//    if (who_is_next(init_info) == 0)
//    {
//        return 0;
//    }
    int replies = 0;
    while(1)
    {
        Message msg;
        printf("I AM HERE %d with replies %d\n", init_info->process_id, replies);
        fflush(stdout);
        if (replies == init_info->processes_count - 2 && who_is_next(init_info) == 0) {        ////если все ответили и мы след.
            //// можем уходить
            return 0;
        }
        int sender_process_id = receive_any(init_info, &msg);
        printf("proc %d recieve type %d\n", init_info->process_id, msg.s_header.s_type);
        fflush(stdout);

        switch (msg.s_header.s_type)
        {
            case CS_RELEASE:
//                printf("release %d, we in %d\n", sender_process_id, init_info->process_id);
                fflush(stdout);
                queue[sender_process_id] = -1;
//                for (int i = 1; i < init_info->processes_count; ++i) {
//                    printf("for process %d - q[%d] = %d\n", init_info->process_id, i, queue[i]);
//                    fflush(stdout);
//                }
                break;
            case CS_REQUEST:                                                ////кто-то хочет в очередь, добавим его
                time++;
//                printf("req for %d with %d\n", init_info->process_id, msg.s_header.s_local_time);
                fflush(stdout);
                queue[sender_process_id] = msg.s_header.s_local_time;
                Message msg_reply = generate_empty_message(get_lamport_time(), MESSAGE_MAGIC, CS_REPLY);
                send(init_info, (local_id)sender_process_id, &msg_reply);
                break;
            case CS_REPLY:
                replies++;
//                printf("replies %d for %d\n", replies, init_info->process_id);
//                fflush(stdout);
                break;
//                while (1) {
//                    printf("wait %d with %d\n", init_info->process_id, get_lamport_time());
//                    fflush(stdout);
//                    if (replies == init_info->processes_count - 2 && who_is_next(init_info) == 0) {        ////если все ответили и мы след.
//                        //// можем уходить
//                        return 0;
//                    }
//                    receive_any(init_info, &msg);
//                    if (msg.s_header.s_type == CS_RELEASE) {
//                        queue[sender_process_id] = -1;
//                        printf("Hey %d\n", queue[sender_process_id]);
//                    }
            default:
                break;
        }
    }
}
int release_cs(const void * self)
{
    InitInfo *initInfo = (InitInfo*)self;
    time++;
    Message msg = generate_empty_message(get_lamport_time(), MESSAGE_MAGIC, CS_RELEASE);
//    queue[initInfo->process_id] = -1;
//    printf("process %d with time %d now %d\n", initInfo->process_id, msg.s_header.s_local_time, queue[initInfo->process_id]);
    send_multicast(initInfo, &msg);
    return 0;
}

int main(int argc, char *argv[]) {
    int processes_count = get_processes_count(argc, argv);

    const struct option options = {"mutexl", 0, &mutexl, 1};
    getopt_long(argc, argv, "", &options, NULL);
    open_log_files();
    InitInfo *initInfo = (InitInfo*)malloc(sizeof(InitInfo));
    initInfo->processes_count = processes_count;
//    initInfo->processes_count = 3; //запуск без консоли

    prepare_decriptors_array(initInfo);
    open_pipes(initInfo);
    create_child_processes(initInfo);
    main_process_get_message(initInfo);

//    printf("done msgs received\n");

    close_log_files();
    free(initInfo);
    return 0;
}
