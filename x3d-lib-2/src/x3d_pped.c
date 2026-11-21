/**
 * @file x3d_pped.c
 * @author Sven Fabricius (sven.fabricius@livediesel.de)
 * @brief
 * @version 1.0
 * @date 2026-11-20
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "x3d.h"
#include "x3d_bytes.h"
#include <stdint.h>
#include <stdlib.h>

#define X3D_PPED_MSG_TYPE_PAIR 0

#define X3D_PPED_NET_REQ       0xc0
#define X3D_PPED_NET_RESP      0x04

void x3d_pped_pairing_req(uint8_t *buffer, uint32_t device_id, uint8_t *msg_no, uint16_t *magic_id)
{
    x3d_msg_t *msg = x3d_proto_init_message(X3D_PPED_MSG_TYPE_PAIR, device_id, X3D_PPED_NET_REQ);

    uint8_t header[] = {0x98, 0x00};
    x3d_proto_prepare_message_header(msg, msg_no, X3D_HEADER_FLAG_NONE, 0x85, header, sizeof(header), magic_id);

    msg->payload_length = 5;
    msg->payload        = calloc(1, msg->payload_length);
    msg->payload[0]     = 0x30 | (X3D_RETRY_MASK & 2);
    msg->payload[1]     = 1;
    msg->payload[2]     = 0xff;
    msg->payload[3]     = 0xff;
    msg->payload[4]     = 0xff;

    x3d_proto_write_to_buffer(buffer, msg);
    free(msg->header_payload);
    free(msg->payload);
    free(msg);
}

void x3d_pped_pairing_resp(uint8_t *buffer, uint32_t device_id, uint8_t *msg_no, uint16_t *magic_id, uint32_t peer_device_id)
{
    x3d_msg_t *msg = x3d_proto_init_message(X3D_PPED_MSG_TYPE_PAIR, device_id, X3D_PPED_NET_RESP);

    uint8_t header[] = {0x98, 0x00};
    x3d_proto_prepare_message_header(msg, msg_no, X3D_HEADER_FLAG_NONE, 0x85, header, sizeof(header), magic_id);

    msg->payload_length = 5;
    msg->payload        = calloc(1, msg->payload_length);
    msg->payload[0]     = 0x30 | 0x08 | (X3D_RETRY_MASK & 2);
    msg->payload[1]     = 5;
    x3d_write_le_u24(peer_device_id, msg->payload, 2);

    x3d_proto_write_to_buffer(buffer, msg);
    free(msg->header_payload);
    free(msg->payload);
    free(msg);
}

int x3d_pped_is_valid_pairing_req(uint8_t *buffer, uint32_t *device_id, uint8_t *retry_cnt)
{
    x3d_msg_t *msg = x3d_proto_parse_verify_msg(buffer);
    if (msg == NULL)
    {
        return -1;
    }

    if (msg->msg_header.msg_type != X3D_PPED_MSG_TYPE_PAIR ||
            msg->msg_header.net != X3D_PPED_NET_REQ ||
            msg->payload_length != 5 ||
            msg->header_payload_length != 5)
    {
        free(msg->header_payload);
        free(msg->payload);
        free(msg);
        return -2;
    }

    if (msg->header_payload[0] != 0x85 ||
            msg->header_payload[1] != 0x98 ||
            msg->payload[1] != 1)
    {
        free(msg->header_payload);
        free(msg->payload);
        free(msg);
        return -3;
    }

    *device_id = msg->msg_header.device_id;
    *retry_cnt = msg->payload[0] & X3D_RETRY_MASK;
    free(msg->header_payload);
    free(msg->payload);
    free(msg);
    return 0;
}

int x3d_pped_is_valid_pairing_resp(uint8_t *buffer, uint32_t device_id, uint32_t *peer_device_id, uint8_t *retry_cnt)
{
    x3d_msg_t *msg = x3d_proto_parse_verify_msg(buffer);
    if (msg == NULL)
    {
        return -1;
    }

    if (msg->msg_header.msg_type != X3D_PPED_MSG_TYPE_PAIR ||
            msg->msg_header.net != X3D_PPED_NET_RESP ||
            msg->payload_length != 5 ||
            msg->header_payload_length != 5)
    {
        free(msg->header_payload);
        free(msg->payload);
        free(msg);
        return -2;
    }

    if (msg->header_payload[0] != 0x85 ||
            msg->header_payload[1] != 0x98 ||
            msg->payload[1] != 5)
    {
        free(msg->header_payload);
        free(msg->payload);
        free(msg);
        return -3;
    }

    uint32_t target_device_id;
    x3d_read_le_u24(&target_device_id, msg->payload, 2);
    if (target_device_id != device_id)
    {
        free(msg->header_payload);
        free(msg->payload);
        free(msg);
        return -4;
    }

    *peer_device_id = msg->msg_header.device_id;
    *retry_cnt      = msg->payload[0] & X3D_RETRY_MASK;
    free(msg->header_payload);
    free(msg->payload);
    free(msg);
    return 0;
}
