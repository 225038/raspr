#ifndef RASPR_IPC_WORKER_H
#define RASPR_IPC_WORKER_H

#include "ipc_pipes.h"
#include "ipc_logs.h"
#include "ipc.h"
#include "ipc_childs_message_worker.h"

extern timestamp_t time;
extern int mutexl;

void create_child_processes(InitInfo* init_info);
void main_process_get_message(InitInfo* init_info);

#endif //RASPR_IPC_WORKER_H
