#ifndef RASPR_IPC_WORKER_H
#define RASPR_IPC_WORKER_H

#include "ipc_pipes.h"
#include "ipc_logs.h"
#include "ipc_childs_message_worker.h"

void create_child_processes(InitInfo* init_info, const balance_t bank_accounts[]);
void main_process_get_message(InitInfo* init_info);

#endif //RASPR_IPC_WORKER_H
