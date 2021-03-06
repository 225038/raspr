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

/**
 * Create needed count of processes. Child processes do their job RETURN??
 * @param init_info
 */
void create_child_processes(InitInfo* init_info)
{
//    printf("Enter create_child_processes\n");
//    fflush(stdout);
    pid_t* output = (pid_t*)malloc(sizeof(pid_t) * init_info->processes_count);
    output[0] = getpid();                                           //add id of main process
    for (local_id i = 1; i < init_info->processes_count; ++i)
    {
        output[i] = fork();
        if (output[i] == 0)
        {
            init_info->process_id = i;
            close_some_pipes(init_info);
//            time++;
            send_start_message_to_all(init_info);

            //Полезная работа
            int iters = i * 5;
            for (int j = 1; j <= iters; j++) {
                char strings[MAX_MESSAGE_LEN];
                sprintf(strings, log_loop_operation_fmt, i, j, iters);
                if (mutexl) {
//                    printf("%d\n", init_info->process_id);
//                    fflush(stdout);
//                    printf("REQUEST %d\n", init_info->process_id);
//                    fflush(stdout);
                    request_cs(init_info);
                    print(strings);
//                    printf("%s\n", strings);
//                    fflush(stdout);
//                    printf("REALISED %d\n", init_info->process_id);
//                    fflush(stdout);
                    release_cs(init_info);
                } else {
                    print(strings);
//                    printf("%s\n", loopStr);
                }
            }

            Message msg;
            int proc = 1;
            send_done_message_to_all(init_info);
            while (1)
            {
                int sender = receive_any(init_info, &msg);
                if (msg.s_header.s_type == DONE) {
                    proc++;
                    if (proc == init_info->processes_count - 1) {
                        send_done_message_to_all(init_info);
                        break;
                    }
                } else if (msg.s_header.s_type == CS_REQUEST) {
                    time++;
                    Message reply = generate_empty_message(get_lamport_time(), MESSAGE_MAGIC, CS_REPLY);
                    send(init_info, (local_id)sender, &reply);
                }
            }

            printf(log_done_fmt, get_lamport_time(), init_info->process_id, init_info->bank_account);
            fflush(stdout);

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
//    fflush(stdout);
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
    receive_from_every_child(init_info, msgs, DONE);
//    printf("received\n");

    while(wait(NULL) > 0) {}                        ////wait for all child processes finish

    write_info_to_events_file(log_received_all_done_fmt, init_info, DONE);
    printf(log_received_all_done_fmt, get_lamport_time(), init_info->process_id);
    fflush(stdout);
    close_its_pipes(init_info);
//    printf("Leave main_process_get_message\n");
}
