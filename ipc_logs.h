//
// Created by aleksandra on 17.04.19.
//

#ifndef RASPR_IPC_LOGS_H
#define RASPR_IPC_LOGS_H

#include "ipc_structs.h"

#define BUF_SIZE 128

int pipes_log_file;
int events_log_file;

void open_log_files();
void close_log_files();
void write_info_to_pipes_log_file(const char * text, int process_id, int from_process, int to_process, int descriptor);
void write_info_to_events_file(const char * text, InitInfo* init_info, MessageType type);
void write_transfer_info_to_events_file(const char * text, TransferOrder* order, MessageType type);

#endif //RASPR_IPC_LOGS_H
