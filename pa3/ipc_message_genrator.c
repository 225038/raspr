#include <stdio.h>              ////for printf
#include <unistd.h>             ////for pipe
#include <string.h>             ////for memset

#include "ipc_message_genrator.h"

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

Message generate_transfer_message(TransferOrder order, timestamp_t time, uint16_t magic_number)
{
//    printf("Enter generate %d, %d, %d\n", order.s_src, order.s_dst, order.s_amount);
    Message msg;
    int length = sizeof(order);
    memcpy(msg.s_payload, &order, length);
    msg.s_header = generate_message_header(magic_number, length, TRANSFER, time);
//    printf("Leave generate\n");
    return msg;
}

Message generate_history_message(BalanceHistory balance_history, timestamp_t time, uint16_t magic_number)
{
    Message msg;
    int length = sizeof(balance_history);
    memcpy(msg.s_payload, &balance_history, length);
    msg.s_header = generate_message_header(magic_number, length, BALANCE_HISTORY, time);
//    printf("Leave generate\n");
    return msg;
}

Message generate_empty_message(timestamp_t time, uint16_t magic_number, MessageType type)
{
    Message msg;
    msg.s_header = generate_message_header(magic_number, 0, type, time);
    return msg;
}
