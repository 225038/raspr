#include <getopt.h>         ////for getopt
#include <stdlib.h>         ////for atoi
#include <stdio.h>          ////for printf
#include <unistd.h>

#include <sys/wait.h>
#include <fcntl.h>              ////for work with file
#include <string.h>             ////for memset

#include "common.h"
#include "ipc_structs.h"
#include "pa2345.h"
#include "ipc_worker.h"
#include "banking.h"

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
    Message ack;

    TransferOrder order;
    order.s_src = src;
    order.s_dst = dst;
    order.s_amount = amount;

    int length = sizeof(order);
    memcpy(msg.s_payload, &order, length);
    msg.s_header = generate_message_header(MESSAGE_MAGIC, length, TRANSFER, get_physical_time());
//    generate_transfer_message(&order, get_physical_time(), MESSAGE_MAGIC);


    send(parent_data, src, &msg);
    write_transfer_info_to_events_file(log_transfer_out_fmt, &order, TRANSFER);
    printf(log_transfer_out_fmt, get_physical_time(), src, amount, dst);

    while (ack.s_header.s_type != ACK)
        receive(parent_data, dst, &ack);
    write_transfer_info_to_events_file(log_transfer_in_fmt, &order, DONE);
    printf(log_transfer_in_fmt, get_physical_time(), dst, amount, src);
//    printf("Leave transfer\n");
}

void amount_transfer(int amount, timestamp_t time, BalanceHistory* balance_history)
{
//    printf("Enter amount_transfer\n");
    balance_t bank_account = balance_history->s_history[balance_history->s_history_len - 1].s_balance;
    for (timestamp_t t = balance_history->s_history_len; t <= time; t++) {
        BalanceState state = {bank_account, t, 0};
        balance_history->s_history[t] = state;
        balance_history->s_history_len++;
    }
    balance_history->s_history[time].s_balance += amount;
//    printf("Leave amount_transfer\n");
}

/**
 * Create needed count of processes. Child processes do their job RETURN??
 * @param init_info
 */
void create_child_processes(InitInfo* init_info, const balance_t* bank_accounts)
{
//    printf("Enter create_child_processes\n");
    BalanceHistory* balance_history = (BalanceHistory*)malloc(sizeof(BalanceHistory));
    pid_t* output = (pid_t*)malloc(sizeof(pid_t) * init_info->processes_count);
    int done_process_counter = 0;
    output[0] = getpid();                                           //add id of main process
    for (local_id i = 1; i < init_info->processes_count; ++i)
    {
        output[i] = fork();
        if (output[i] == 0)
        {
//            printf("Enter create_child_processes\n");
            // code for child
            init_info->process_id = i;
            init_info->bank_account = bank_accounts[i];
            balance_history->s_id = init_info->process_id;
            balance_history->s_history_len = 1;
            //Начальное состояние счета
            balance_history->s_history[0] = (BalanceState){init_info->bank_account, get_physical_time(), 0};
            close_some_pipes(init_info);
            sent_start_message_to_all(init_info);

            Message msg_received;
            //Полезная работа
            while (done_process_counter != init_info->processes_count - 1)
            {
//                printf("done %d processes\n", done_process_counter);
//                printf("start receive %d\n", init_info->process_id);
                receive(init_info, 0, &msg_received);
                printf("process %d receive msg with type %d\n", init_info->process_id, msg_received.s_header.s_type);
                switch (msg_received.s_header.s_type)
                {
                    case TRANSFER:
                    {
                        printf("in TRANSFER %d\n", init_info->process_id);
                        TransferOrder* order = (TransferOrder*) msg_received.s_payload;
                        if (order->s_src == i) {
                            msg_received.s_header.s_local_time = get_physical_time();
                            amount_transfer(-order->s_amount, msg_received.s_header.s_local_time, balance_history);
                            send(init_info, order->s_dst, &msg_received);
                        } else if (order->s_dst == i) {
                            amount_transfer(-order->s_amount, msg_received.s_header.s_local_time, balance_history);
//                            printf("history written %d %d\n", history.s_history[history.s_history_len - 1].s_balance,
//                                   history.s_history[history.s_history_len - 1].s_time);
//                            fprintf(logfile, log_transfer_in_fmt, get_physical_time(), order.s_src, order.s_amount,
//                                    order.s_dst);

                            Message ack;
                            ack = generate_empty_message(get_physical_time(), MESSAGE_MAGIC, ACK);
                            send(init_info, 0, &ack);
                        }
                        break;
                    }
                    case STOP:
                    {
                        printf("in STOP %d\n", init_info->process_id);
                        Message msg_done = generate_message(DONE, init_info, get_physical_time(), MESSAGE_MAGIC);
                        send_multicast(init_info, &msg_done);
                        write_info_to_events_file(log_done_fmt, init_info, DONE);
                        printf(log_done_fmt, get_physical_time(), init_info->process_id, init_info->bank_account);
                        break;
                    }
                    case DONE:
                    {
                        done_process_counter++;
                    }
                }
            }
            sent_done_message_to_all(init_info);
            // closing this process's pipes in other processes
            close_its_pipes(init_info);
            exit(0);
        } else if (output[i] < 0)
        {
            // code for error
            printf("Can not fork\n");
        }
        // code for parent, so if it is parent it will go to the next step of cycle
    }
    free(output);
//    printf("Leave create_child_processes\n");
}

/**
 * Main process watch for his children
 * @param init_info
 */
void main_process_get_message(InitInfo* init_info)
{
//    printf("Enter main_process_get_message\n");
    init_info->process_id = 0;                  //// switch to main process

    close_some_pipes(init_info);                //// close pipes which not connect to this process

    Message msg_received;
    receive_any(init_info, &msg_received);
    write_info_to_events_file(log_received_all_started_fmt, init_info, STARTED);
    printf(log_received_all_started_fmt, get_physical_time(), init_info->process_id);

    //Переводы: processes_count - 1 - т.к. там количество всех процессов, включая родительский 0,
    //а нам нужны только дочерние
    bank_robbery(init_info, init_info->processes_count - 1);

    Message msg_stop = generate_message(STOP, init_info, get_physical_time(), MESSAGE_MAGIC);
    generate_empty_message(get_physical_time(), MESSAGE_MAGIC, STOP);
    send_multicast(init_info, &msg_stop);

    memset(&msg_received, 0, sizeof(Message));      ////clean the message
    receive_any(init_info, &msg_received);
    write_info_to_events_file(log_received_all_done_fmt, init_info, DONE);
    printf(log_received_all_done_fmt, get_physical_time(), init_info->process_id);

    while(wait(NULL) > 0) {}                        ////wait for all child processes finish
    close_its_pipes(init_info);
    printf("Leave main_process_get_message\n");
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

//    Message history_msgs[initInfo->processes_count + 1];
//    receive_any(initInfo, history_msgs);

//    AllHistory all_history;
//    all_history.s_history_len = initInfo->processes_count;
//    for (int i = 0; i < initInfo->processes_count; i++) {
//        memcpy(&all_history.s_history[i], &history_msgs[i + 1].s_payload, sizeof(BalanceHistory));
//        printf("%d\n", all_history.s_history[i].s_history[2].s_balance);
//    }
//    print_history(&all_history);

    close_log_files();
    free(initInfo);
    return 0;
}
