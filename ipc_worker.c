#include <stdio.h>              ////for printf
#include <stdlib.h>             ////for malloc
#include <unistd.h>             ////for pipe
#include <fcntl.h>              ////for work with file
#include <string.h>             ////for memset
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "ipc_worker.h"
#include "ipc.h"
#include "common.h"
#include "pa2345.h"
#include "banking.h"

#define FD_SIZE 2
#define BUF_SIZE 128

void open_log_files()
{
    pipes_log_file = open(pipes_log, O_WRONLY | O_APPEND | O_CREAT | O_TRUNC, 777);
    events_log_file = open(events_log, O_WRONLY | O_APPEND | O_CREAT | O_TRUNC, 777);
}

void close_log_files()
{
    close(pipes_log_file);
    close(events_log_file);
}

/** This function frees memory which was allocated
 * @param ptr   Pointer to anything we want to free
*/
void free_memory(void * ptr)
{
    free(ptr);
}

/**
 * Prepare array before filling it with descriptors. Allocate memory for structures' array
 * @param init_info     Pointer to structure with descriptors' array we need to allocate memory
 */
void prepare_decriptors_array(InitInfo* init_info)
{
    for (int i = 0; i < init_info->processes_count; ++i)
    {
        for (int j = 0; j < init_info->processes_count; ++j)
        {
            if (i != j)                         ////because we do not need descriptor to itself
            {
                init_info->descriptors[i][j] = (ConnectionInfo*)malloc(sizeof(ConnectionInfo));
            }
        }
    }
}

/**
 * Open n*(n-1) pipes. Write info about it to file pipes.log
 * @param init_info     Pointer to structure with all needed info
 */
void open_pipes(InitInfo* init_info)
{
    int fd[FD_SIZE];                                                 ////here we will put read and write descriptors
    for (int i = 0; i < init_info->processes_count; ++i)
    {
        for (int j = 0; j < init_info->processes_count; ++j)
        {
            if (i != j)
            {
                int return_status = pipe(fd);
                if (return_status < 0)
                {
                    perror("Can not open pipe");
                    return;
                }
                //// now in fd we have descriptors for pipe. in fd[0] we have read end, in f[1] we have write end
                init_info->descriptors[j][i]->read_fd = fd[0];
                init_info->descriptors[i][j]->write_fd = fd[1];

                write_info_to_pipes_log_file(open_pipe, init_info->process_id, i, j, init_info->descriptors[i][j]->write_fd);
//                printf("write %d %d fd = %d\n", i, j, init_info->descriptors[i][j]->write_fd);
                write_info_to_pipes_log_file(open_pipe, init_info->process_id, j, i, init_info->descriptors[j][i]->read_fd);
            }
        }
//    printf("Leave open_pipes\n");
    }
}


/**
 * Function for closing unnessesary pipes. There are pipes which are not connected to current process.
 * @param init_info     Pointer to structure with all needed info
 */
void close_some_pipes(InitInfo* init_info)
{
//    printf("Leave close_some_pipes\n");

    for (int j = 0; j < init_info->processes_count; ++j)
    {
        if (j != init_info->process_id)                 //// if j == process_id then pipe is needed and connected to current process
        {
            for (int k = 0; k < init_info->processes_count; ++k)
            {
                if (j != k)                             //// because here we do not have anything
                {
                    write_info_to_pipes_log_file(close_pipe, init_info->process_id, j, k, init_info->descriptors[j][k]->write_fd);
                    write_info_to_pipes_log_file(close_pipe, init_info->process_id, j, k, init_info->descriptors[j][k]->read_fd);

                    close(init_info->descriptors[j][k]->read_fd);
                    close(init_info->descriptors[j][k]->write_fd);
                    free_memory(init_info->descriptors[j][k]);
                }

            }
        }
    }
//    printf("Leave close_some_pipes\n");
}

/**
 * Function for closing pipes connected to this process. Call after process did useful job.
 * @param init_info     Pointer to structure with all needed info
 */
void close_its_pipes(InitInfo* init_info)
{
    for (local_id i = 0; i < init_info->processes_count; ++i)
    {
        if (i != init_info->process_id)                 ////to avoid situations like [i][i]. If proc_id == 2, we will remove [2][1] and [2][0]
        {
            write_info_to_pipes_log_file(close_pipe, init_info->process_id, init_info->process_id, i,
                                         init_info->descriptors[init_info->process_id][i]->write_fd);
            write_info_to_pipes_log_file(close_pipe, init_info->process_id, init_info->process_id, i,
                                         init_info->descriptors[init_info->process_id][i]->read_fd);

            close(init_info->descriptors[init_info->process_id][i]->read_fd);
            close(init_info->descriptors[init_info->process_id][i]->write_fd);
            free(init_info->descriptors[init_info->process_id][i]);
        }
    }
//    printf("Leave close_its_pipes\n");
}

///**
// * Create needed count of processes. Child processes do their job RETURN??
// * @param init_info
// */
//void create_child_processes(InitInfo* init_info, const balance_t* bank_accounts)
//{
////    printf("Enter create_child_processes\n");
//    pid_t* output = (pid_t*)malloc(sizeof(pid_t) * init_info->processes_count);
//    output[0] = getpid();                                           ////add id of main process
//    for (local_id i = 1; i < init_info->processes_count; ++i) {
//        output[i] = fork();
//        if (output[i] == 0)
//        {
//            //// code for child
//            init_info->process_id = i;
//            init_info->bank_account = bank_accounts[i];
//            close_some_pipes(init_info);
//            sent_start_message_to_all(init_info);
//            sent_done_message_to_all(init_info);
//            // closing this process's pipes in other processes
//            close_its_pipes(init_info);
//            exit(0);
//        } else if (output[i] < 0)
//        {
//            //// code for error
//            printf("Can not fork\n");
//        }
//        //// code for parent, so if it is parent it will go to the next step of cycle
//    }
//    free(output);
////    printf("Leave create_child_processes\n");
//}

/**
 * Write needed information to events.log file
 * @param text              String to write
 * @param init_info        In which process event happened
 * @param type              Type of event (START, DONE)
 */
void write_info_to_events_file(const char * text, InitInfo* init_info, MessageType type)
{
    char buffer[BUF_SIZE];
    int length = 0;
    if (type == STARTED)
    {
        length = sprintf(buffer, text, get_physical_time(), init_info->process_id,
                getpid(), getppid(), init_info->bank_account);
    } else if (type == DONE)
    {
        length = sprintf(buffer, text, get_physical_time(), init_info->process_id, init_info->bank_account);
    }
    write(events_log_file, buffer, length);
}

void write_transfer_info_to_events_file(const char * text, TransferOrder* order, MessageType type)
{
    char buffer[BUF_SIZE];
    int length = 0;
    if (type == TRANSFER)
    {
        length = sprintf(buffer, text, get_physical_time(), order->s_src, order->s_amount, order->s_dst);
    } else if (type == DONE)
    {
        length = sprintf(buffer, text, get_physical_time(), order->s_dst, order->s_amount, order->s_src);
    }
    write(events_log_file, buffer, length);
}

/**
 * Write needed information to pipes.log file
 * @param text              String to write
 * @param process_id        In which process pipe was opened
 * @param from_process      From which process
 * @param to_process        To which process
 * @param descriptor        Descriptor number
 */
void write_info_to_pipes_log_file(const char * text, int process_id, int from_process, int to_process, int descriptor)
{
    char buffer[BUF_SIZE];
    int length = sprintf(buffer, text,
                         process_id, from_process, to_process, descriptor);
    write(pipes_log_file, buffer, length);
}

/**
 * In this function process tells to other that it has started. After that it is waiting for messages from others.
 * @param init_info
 */
void sent_start_message_to_all(InitInfo * init_info)
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

//    Message msg_received;
//    receive_any(init_info, &msg_received);
//    write_info_to_events_file(log_received_all_started_fmt, init_info, STARTED);
//    printf(log_received_all_started_fmt, get_physical_time(), init_info->process_id);
//    printf("Leave sent_start_message_to_all\n");
}

/**
 * In this function process tells to other that it has done. After that it is waiting for messages from others.
 * @param init_info
 */
void sent_done_message_to_all(InitInfo *init_info)
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
    Message msg_received;
    receive_any(init_info, &msg_received);
}

/**
 * Fill all fields of message header
 * @param magic_number
 * @param payload_length        Length of buffer
 * @param type                  Type of message
 * @param local_time            Current time
 * @return filled message header
 */
MessageHeader generate_message_header(uint16_t magic_number, uint16_t payload_length, int16_t type, timestamp_t local_time)
{
    MessageHeader header;
    header.s_magic = magic_number;
    header.s_payload_len = payload_length;
    header.s_type = type;
    header.s_local_time = local_time;
    return header;
}

//Generate message. Message depends on type, so we count length to put it in header
/**
 * Generate message header and message
 * @param msg_type          Type of message
 * @param process_id        Current process
 * @param time              Current time
 * @param magic_number
 * @return prepared message
 */
Message generate_message(uint16_t msg_type, InitInfo* initInfo, timestamp_t time, uint16_t magic_number)
{
    Message msg;
    int length = 0;
    if(msg_type == STARTED)
    {
        length = sprintf(msg.s_payload,  log_started_fmt, time,
                initInfo->process_id, getpid(), getppid(), initInfo->bank_account);
    } else if (msg_type == DONE)
    {
        length = sprintf(msg.s_payload,  log_done_fmt, time, initInfo->process_id, initInfo->bank_account);
    }
    msg.s_header = generate_message_header(magic_number, length, msg_type, time);
    return msg;
}

//Message generate_transfer_message(TransferOrder *order, timestamp_t time, uint16_t magic_number)
//{
//    printf("Enter generate");
//    Message msg;
//    int length = sizeof(TransferOrder);
//    memcpy(msg.s_payload, order, length);
//    msg.s_header = generate_message_header(magic_number, length, TRANSFER, time);
//    printf("Leave generate");
//    return msg;
//}

Message generate_empty_message(timestamp_t time, uint16_t magic_number, MessageType type)
{
    Message msg;
    msg.s_header = generate_message_header(magic_number, 0, type, time);
    return msg;
}
