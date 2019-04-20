#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>              ////for printf
#include "ipc_logs.h"
#include "common.h"

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

