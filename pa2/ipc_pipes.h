//
// Created by aleksandra on 16.04.19.
//

#ifndef RASPR_IPC_PIPES_H
#define RASPR_IPC_PIPES_H

#include "ipc_structs.h"
#include "ipc_logs.h"

void close_some_pipes(InitInfo* init_info);
void close_its_pipes(InitInfo* init_info);
void prepare_decriptors_array(InitInfo* init_info);
void open_pipes(InitInfo* init_info);

#endif //RASPR_IPC_PIPES_H
