#ifndef RASPR_IPC_WORKER_H
#define RASPR_IPC_WORKER_H

#include "ipc_structs.h"

int pipes_log_file;
int events_log_file;
void open_log_files();
void close_log_files();
void write_info_to_pipes_log_file(const char * text, int process_id, int from_process, int to_process, int descriptor);
void write_info_to_events_file(const char * text, InitInfo* init_info, MessageType type);
void write_transfer_info_to_events_file(const char * text, TransferOrder* order, MessageType type);

void close_some_pipes(InitInfo* init_info);
void close_its_pipes(InitInfo* init_info);
void prepare_decriptors_array(InitInfo* init_info);
void open_pipes(InitInfo* init_info);

//void create_child_processes(InitInfo* init_info, const balance_t bank_accounts[]);
//void main_process_get_message(InitInfo* init_info);

MessageHeader generate_message_header(uint16_t magic_number, uint16_t payload_length, int16_t type, timestamp_t local_time);
Message generate_message(uint16_t msg_type, InitInfo* initInfo, timestamp_t time, uint16_t magic_number);
Message generate_transfer_message(TransferOrder *order, timestamp_t time, uint16_t magic_number);
Message generate_empty_message(timestamp_t time, uint16_t magic_number, MessageType type);

void sent_start_message_to_all(InitInfo* init_info);
void sent_done_message_to_all(InitInfo *init_info);
#endif //RASPR_IPC_WORKER_H
