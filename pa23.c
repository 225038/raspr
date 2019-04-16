#include <getopt.h>         ////for getopt
#include <stdlib.h>         ////for atoi
#include <stdio.h>          ////for printf

#include "ipc_worker.h"

//Parse argv and get param of -p key; +1 in return because general number of processes includes parent process.
int get_processes_count(int argc, char* argv[])
{
    int output = 0;             //return number of processes
    int tmp_var;               //to keep what getopt() returns
    while ((tmp_var = getopt(argc, argv, "p:")) != -1)
    {
        if (tmp_var == 'p')
        {
            output = atoi(optarg);
        }
    }
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

    msg = generate_transfer_message(order, get_physical_time(), MESSAGE_MAGIC);
    send(parent_data, src, &msg);
    write_transfer_info_to_events_file(log_transfer_out_fmt, &order, TRANSFER);
    printf(log_transfer_out_fmt, get_physical_time(), src, amount, dst);

    while (1)
    {
//        printf("type %d\n", ack.s_header.s_type);
        receive(parent_data, dst, &msg);
        if (msg.s_header.s_type == ACK){
            goto leave_transfer;
        }
    }
    leave_transfer:
    write_transfer_info_to_events_file(log_transfer_in_fmt, &order, DONE);
    printf(log_transfer_in_fmt, get_physical_time(), dst, amount, src);
//    printf("Leave transfer\n");
}

int main(int argc, char *argv[]) {
    int processes_count = get_processes_count(argc, argv);
    balance_t bank_accounts[processes_count];
    for (int i = 1; i < processes_count; ++i)
    {
        bank_accounts[i] = atoi(argv[i + 2]);
//        printf("Баланс на счету %d: %d$\n", i, bank_accounts[i]);
    }
    open_log_files();
    InitInfo *initInfo = (InitInfo*)malloc(sizeof(InitInfo));
    initInfo->processes_count = processes_count;
//    initInfo->processes_count = 3; //запуск без консоли

    prepare_decriptors_array(initInfo);
    open_pipes(initInfo);
    create_child_processes(initInfo, bank_accounts);
    main_process_get_message(initInfo);

//    printf("done msgs received\n");

    close_log_files();
    free(initInfo);
    return 0;
}
