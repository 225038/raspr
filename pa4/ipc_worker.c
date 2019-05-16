#include <stdlib.h>         ////for atoi
#include <stdio.h>          ////for printf
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>         ////for memset

#include "ipc_worker.h"

timestamp_t time = 0;

timestamp_t get_lamport_time() {
    return time;
}

void amount_transfer(int amount, timestamp_t t_time, BalanceHistory *balance_history, InitInfo *init_info)
{
    balance_t bank_account = balance_history->s_history[balance_history->s_history_len - 1].s_balance;
    //Заполняем balance_states, во время которых обменивались другие процессы, нашим старым значением счета
    for (int i = balance_history->s_history_len; i <= t_time; i++) {
        BalanceState balance_state = {bank_account, i, 0};
        balance_history->s_history[i] = balance_state;
        balance_history->s_history_len++;
    }
    //Теперь для последнего balance_state увеличиваем/уменьшаем значение на указанную нам сумму
    balance_history->s_history[t_time].s_balance += amount;
    init_info->bank_account += amount;
    if (amount > 0) {
        balance_history->s_history[t_time - 1].s_balance_pending_in = (balance_t) amount;       //когда отправили
        balance_history->s_history[t_time - 2].s_balance_pending_in = (balance_t) amount;       //когда получили transfer
    }
//    printf("Leave\n");
}

/**
 * Create needed count of processes. Child processes do their job RETURN??
 * @param init_info
 */
void create_child_processes(InitInfo* init_info, const balance_t* bank_accounts)
{
//    printf("Enter create_child_processes\n");
    BalanceHistory balance_history;
    pid_t* output = (pid_t*)malloc(sizeof(pid_t) * init_info->processes_count);
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
            balance_history.s_id = init_info->process_id;
            balance_history.s_history_len = 1;
            //Начальное состояние счета
            balance_history.s_history[0] = (BalanceState){init_info->bank_account, get_lamport_time(), 0};
            close_some_pipes(init_info);
            send_start_message_to_parent(init_info);

            Message msg_received;
            //Полезная работа
            while (1)
            {
//                printf("start receive %d\n", init_info->process_id);
                receive_any(init_info, &msg_received);
//                printf("process %d receive msg with type %d\n", init_info->process_id, msg_received.s_header.s_type);
                switch (msg_received.s_header.s_type)
                {
                    case TRANSFER:
                    {
//                        printf("in TRANSFER %d\n", init_info->process_id);
                        TransferOrder* order = (TransferOrder*) msg_received.s_payload;
//                        printf("src %d dst %d amount %d\n", order->s_src, order->s_dst, order->s_amount);
                        if (order->s_src == i) {
                            amount_transfer(-order->s_amount,
                                            get_lamport_time(), &balance_history, init_info);
//                            time++;
                            msg_received.s_header.s_local_time = get_lamport_time();
                            send(init_info, order->s_dst, &msg_received);
                        } else if (order->s_dst == i) {
                            amount_transfer(order->s_amount,
                                            get_lamport_time(), &balance_history, init_info);
                            Message ack;
                            ack = generate_empty_message(get_lamport_time(), MESSAGE_MAGIC, ACK);
                            send(init_info, 0, &ack);
                        }
//                        printf("Leave TRANSFER %d\n", init_info->process_id);
                        break;
                    }
                    case STOP:
                    {
//                        printf("in STOP %d\n", init_info->process_id);
                        //Без этого у процессов, не участвовавших в последней транзакции - 0 в истории
                        amount_transfer(0, get_lamport_time(), &balance_history, init_info);
                        send_history_message_to_parent(init_info, balance_history);
                        receive_any(init_info, &msg_received);
                        send_done_message_to_parent(init_info);
                        receive_any(init_info, &msg_received);
                        goto leave_child_process;
                    }
                }
            }
            leave_child_process:

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

void print_transactions_history(InitInfo* initInfo, Message* msgs)
{
//    printf("history msgs received\n");

    AllHistory all_history;
    all_history.s_history_len = initInfo->processes_count - 1;
    for (int i = 1; i < initInfo->processes_count; i++) {

        memcpy(&all_history.s_history[(i-1)], &msgs[(i-1)].s_payload, msgs[(i-1)].s_header.s_payload_len);
//        for (int j = 0; j < initInfo->processes_count; j++)
//        {
//            printf("for account %d - balance %d = %d\n", all_history.s_history[i-1].s_id, j,
//                   all_history.s_history[i-1].s_history[j].s_balance);
//        }
    }
//    printf("history msgs will print\n");
    print_history(&all_history);
    fflush(stdout);
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

    Message msgs[init_info->processes_count - 1];
    receive_from_every_child(init_info, msgs, STARTED);
    write_info_to_events_file(log_received_all_started_fmt, init_info, STARTED);
    printf(log_received_all_started_fmt, get_lamport_time(), init_info->process_id);
    fflush(stdout);
    //Переводы: processes_count - 1 - т.к. там количество всех процессов, включая родительский 0,
    //а нам нужны только дочерние
    bank_robbery(init_info, init_info->processes_count - 1);

    Message msg_stop = generate_empty_message(get_lamport_time(), MESSAGE_MAGIC, STOP);
    send_multicast(init_info, &msg_stop);

    Message history_messages[init_info->processes_count - 1];
    receive_from_every_child(init_info, history_messages, BALANCE_HISTORY);
    send_multicast(init_info, &msg_stop);
//    for (int i = 0; i < init_info->processes_count - 1; ++i)
//    {
//        printf("received %d from %d\n", history_messages[i].s_header.s_type, i + 1);
//    }

    receive_from_every_child(init_info, msgs, DONE);
    send_multicast(init_info, &msg_stop);
//    printf("received\n");


    while(wait(NULL) > 0) {}                        ////wait for all child processes finish

    write_info_to_events_file(log_received_all_done_fmt, init_info, DONE);
    printf(log_received_all_done_fmt, get_lamport_time(), init_info->process_id);
    fflush(stdout);
    print_transactions_history(init_info, history_messages);
    close_its_pipes(init_info);
//    printf("Leave main_process_get_message\n");
}
