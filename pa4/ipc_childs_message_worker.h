#ifndef RASPR_IPC_CHILDS_MESSAGE_WORKER_H
#define RASPR_IPC_CHILDS_MESSAGE_WORKER_H

#include "ipc_structs.h"
#include "ipc_message_genrator.h"

void send_start_message_to_all(InitInfo *init_info);
void send_done_message_to_all(InitInfo *init_info);
void send_history_message_to_parent(InitInfo *init_info, BalanceHistory balance_history);
int receive_from_every_child(void *self, Message *msgs, MessageType type);

#endif //RASPR_IPC_CHILDS_MESSAGE_WORKER_H
