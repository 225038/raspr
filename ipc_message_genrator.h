#ifndef RASPR_IPC_MESSAGE_GENRATOR_H
#define RASPR_IPC_MESSAGE_GENRATOR_H

#include "ipc_structs.h"
#include "pa2345.h"

MessageHeader generate_message_header(uint16_t magic_number, uint16_t payload_length, int16_t type, timestamp_t local_time);
Message generate_message(uint16_t msg_type, InitInfo* initInfo, timestamp_t time, uint16_t magic_number);
Message generate_history_message(BalanceHistory balance_history, timestamp_t time, uint16_t magic_number);
Message generate_transfer_message(TransferOrder order, timestamp_t time, uint16_t magic_number);
Message generate_empty_message(timestamp_t time, uint16_t magic_number, MessageType type);

#endif //RASPR_IPC_MESSAGE_GENRATOR_H
