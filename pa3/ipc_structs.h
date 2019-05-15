#ifndef RASPR_IPC_STRUCTS_H
#define RASPR_IPC_STRUCTS_H

#include "ipc.h"
#include "banking.h"

typedef struct
{
    int read_fd;
    int write_fd;
} ConnectionInfo;

typedef struct
{
    int processes_count;
    local_id process_id;
    balance_t bank_account;
    ConnectionInfo *descriptors[MAX_PROCESS_ID][MAX_PROCESS_ID];
} InitInfo;

static const char * const open_pipe = "For process %d from process %d to %d opened pipe. Pipe with descriptor %d\n";
static const char * const close_pipe = "For process %d from process %d to %d closed pipe. Pipe with descriptor %d\n";
#endif
