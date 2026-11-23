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

#include "x3d_const.h"
#include "x3d_bytes.h"
#include "x3d_types.h"
#include "x3d_proto.h"
#include "x3d_pped.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define X3D_PPED_MSG_TYPE_PAIR     0
#define X3D_PPED_MSG_TYPE_REGISTER 5

#define X3D_PPED_NET_REQ           0xc0
#define X3D_PPED_NET_RESP          0x04

void x3d_pped_pairing_req(uint8_t *buffer, x3d_context_t *ctx)
{
    x3d_msg_t *msg = x3d_proto_init_message(X3D_PPED_MSG_TYPE_PAIR, ctx->device_id, X3D_PPED_NET_REQ);

    uint8_t header[] = {0x98, 0x00};
    x3d_proto_prepare_message_header(msg, &ctx->msg_no, X3D_HEADER_FLAG_NONE, 0x85, header, sizeof(header), &ctx->magic_id);

    msg->payload_length                 = 5;
    msg->payload[X3D_PAYLOAD_IDX_RETRY] = 0x30 | (X3D_RETRY_MASK & 2);
    msg->payload[1]                     = 1;
    x3d_write_le_u24(0xffffff, msg->payload, X3D_PPED_IDX_PAYLOAD_TARGET_DEVICE_ID);

    x3d_proto_write_to_buffer(buffer, msg);
    free(msg);
}

int x3d_pped_parse_pairing_req(uint8_t *buffer, uint32_t *peer_device_id, uint8_t *retry_cnt, uint16_t *magic_id)
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
        free(msg);
        return -2;
    }

    if (msg->header_payload[0] != 0x85 ||
            msg->header_payload[1] != 0x98 ||
            msg->payload[1] != 1)
    {
        free(msg);
        return -3;
    }

    if ((msg->payload[X3D_PAYLOAD_IDX_RETRY] & X3D_RETRY_MASK_INV) != 0x30)
    {
        return -4;
    }

    *peer_device_id = msg->msg_header.device_id;
    *retry_cnt      = msg->payload[X3D_PAYLOAD_IDX_RETRY] & X3D_RETRY_MASK;
    *magic_id       = msg->header_magic;
    free(msg);
    return 0;
}

void x3d_pped_pairing_resp(uint8_t *buffer, x3d_context_t *ctx, uint32_t peer_device_id)
{
    x3d_msg_t *msg = x3d_proto_init_message(X3D_PPED_MSG_TYPE_PAIR, ctx->device_id, X3D_PPED_NET_RESP);

    uint8_t header[] = {0x98, 0x00};
    x3d_proto_prepare_message_header(msg, &ctx->msg_no, X3D_HEADER_FLAG_NONE, 0x85, header, sizeof(header), &ctx->magic_id);

    msg->payload_length                 = 5;
    msg->payload[X3D_PAYLOAD_IDX_RETRY] = 0x30 | 0x08 | (X3D_RETRY_MASK & 2);
    msg->payload[1]                     = 5;
    x3d_write_le_u24(peer_device_id, msg->payload, X3D_PPED_IDX_PAYLOAD_TARGET_DEVICE_ID);

    x3d_proto_write_to_buffer(buffer, msg);
    free(msg);
}

int x3d_pped_parse_pairing_resp(uint8_t *buffer, x3d_context_t *ctx, uint32_t *peer_device_id, uint8_t *retry_cnt, uint16_t *magic_id)
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
        free(msg);
        return -2;
    }

    if (msg->header_payload[0] != 0x85 ||
            msg->header_payload[1] != 0x98 ||
            msg->payload[1] != 5)
    {
        free(msg);
        return -3;
    }

    uint32_t target_device_id;
    x3d_read_le_u24(&target_device_id, msg->payload, X3D_PPED_IDX_PAYLOAD_TARGET_DEVICE_ID);
    if (target_device_id != ctx->device_id)
    {
        free(msg);
        return -4;
    }

    if ((msg->payload[X3D_PAYLOAD_IDX_RETRY] & X3D_RETRY_MASK_INV) != 0x38)
    {
        return -5;
    }

    *peer_device_id = msg->msg_header.device_id;
    *retry_cnt      = msg->payload[X3D_PAYLOAD_IDX_RETRY] & X3D_RETRY_MASK;
    *magic_id       = msg->header_magic;
    free(msg);
    return 0;
}

void x3d_pped_register_req(uint8_t *buffer, x3d_context_t *ctx, x3d_pped_register_t *reg_data)
{
    x3d_msg_t *msg = x3d_proto_init_message(X3D_PPED_MSG_TYPE_REGISTER, ctx->device_id, X3D_PPED_NET_REQ);

    uint8_t header[] = {0x98, 0x00};
    x3d_proto_prepare_message_header(msg, &ctx->msg_no, X3D_HEADER_FLAG_NONE, 0x05, header, sizeof(header), &ctx->magic_id);

    msg->payload_length = 7 + reg_data->reg_payload_length;

    msg->payload[X3D_PAYLOAD_IDX_RETRY] = 0x30 | (X3D_RETRY_MASK & 2);
    msg->payload[1]                     = 0;
    reg_data->is_response               = false;
    x3d_write_le_u24(reg_data->peer_device_id, msg->payload, X3D_PPED_IDX_PAYLOAD_TARGET_DEVICE_ID);
    x3d_write_le_u16(reg_data->reg_u, msg->payload, X3D_PPED_IDX_PAYLOAD_REG);
    memcpy(&msg->payload[X3D_PPED_IDX_PAYLOAD_REG_PAYLOAD], reg_data->reg_payload, reg_data->reg_payload_length);

    x3d_proto_write_to_buffer(buffer, msg);
    free(msg);
}

int x3d_pped_parse_register_req(uint8_t *buffer, x3d_context_t *ctx, x3d_pped_register_t *reg_data)
{
    x3d_msg_t *msg = x3d_proto_parse_verify_msg(buffer);
    if (msg == NULL)
    {
        return -1;
    }

    if (msg->msg_header.msg_type != X3D_PPED_MSG_TYPE_REGISTER ||
            msg->msg_header.net != X3D_PPED_NET_REQ ||
            msg->payload_length < 7 ||
            msg->header_payload_length != 5)
    {
        free(msg);
        return -2;
    }

    if (msg->header_payload[0] != 0x05 ||
            msg->header_payload[1] != 0x98 ||
            msg->payload[1] != 0)
    {
        free(msg);
        return -3;
    }

    uint32_t target_device_id;
    x3d_read_le_u24(&target_device_id, msg->payload, X3D_PPED_IDX_PAYLOAD_TARGET_DEVICE_ID);
    if (target_device_id != ctx->device_id)
    {
        free(msg);
        return -4;
    }

    if ((msg->payload[X3D_PAYLOAD_IDX_RETRY] & X3D_RETRY_MASK_INV) != 0x30)
    {
        return -5;
    }

    reg_data->peer_device_id = msg->msg_header.device_id;
    reg_data->retry_cnt      = msg->payload[X3D_PAYLOAD_IDX_RETRY] & X3D_RETRY_MASK;
    reg_data->magic_id       = msg->header_magic;

    x3d_read_le_u16(&reg_data->reg_u, msg->payload, X3D_PPED_IDX_PAYLOAD_REG);
    if (msg->payload_length > X3D_PPED_IDX_PAYLOAD_REG_PAYLOAD)
    {
        reg_data->reg_payload_length = msg->payload_length - X3D_PPED_IDX_PAYLOAD_REG_PAYLOAD;
        memcpy(reg_data->reg_payload, &msg->payload[X3D_PPED_IDX_PAYLOAD_REG_PAYLOAD], reg_data->reg_payload_length);
    }
    else
    {
        reg_data->reg_payload_length = 0;
    }

    return reg_data->is_response ? -6 : 0;
}

void x3d_pped_register_resp(uint8_t *buffer, x3d_context_t *ctx, x3d_pped_register_t *reg_data)
{
    x3d_msg_t *msg = x3d_proto_init_message(X3D_PPED_MSG_TYPE_REGISTER, ctx->device_id, X3D_PPED_NET_RESP);

    uint8_t header[] = {0x98, 0x00};
    x3d_proto_prepare_message_header(msg, &ctx->msg_no, X3D_HEADER_FLAG_NONE, 0x05, header, sizeof(header), &ctx->magic_id);

    msg->payload_length                 = 7 + reg_data->reg_payload_length;
    msg->payload[X3D_PAYLOAD_IDX_RETRY] = 0x30 | 0x08 | (X3D_RETRY_MASK & 2);
    msg->payload[1]                     = 0;
    reg_data->is_response               = true;
    x3d_write_le_u24(reg_data->peer_device_id, msg->payload, X3D_PPED_IDX_PAYLOAD_TARGET_DEVICE_ID);
    x3d_write_le_u16(reg_data->reg_u, msg->payload, X3D_PPED_IDX_PAYLOAD_REG);
    memcpy(&msg->payload[X3D_PPED_IDX_PAYLOAD_REG_PAYLOAD], reg_data->reg_payload, reg_data->reg_payload_length);

    x3d_proto_write_to_buffer(buffer, msg);
    free(msg);
}

int x3d_pped_parse_register_resp(uint8_t *buffer, x3d_context_t *ctx, x3d_pped_register_t *reg_data)
{
    x3d_msg_t *msg = x3d_proto_parse_verify_msg(buffer);
    if (msg == NULL)
    {
        return -1;
    }

    if (msg->msg_header.msg_type != X3D_PPED_MSG_TYPE_REGISTER ||
            msg->msg_header.net != X3D_PPED_NET_RESP ||
            msg->payload_length < 7 ||
            msg->header_payload_length != 5)
    {
        free(msg);
        return -2;
    }

    if (msg->header_payload[0] != 0x05 ||
            msg->header_payload[1] != 0x98 ||
            msg->payload[1] != 0)
    {
        free(msg);
        return -3;
    }

    uint32_t target_device_id;
    x3d_read_le_u24(&target_device_id, msg->payload, X3D_PPED_IDX_PAYLOAD_TARGET_DEVICE_ID);
    if (target_device_id != ctx->device_id)
    {
        free(msg);
        return -4;
    }

    if ((msg->payload[X3D_PAYLOAD_IDX_RETRY] & X3D_RETRY_MASK_INV) != 0x38)
    {
        return -6;
    }

    reg_data->peer_device_id = msg->msg_header.device_id;
    reg_data->retry_cnt      = msg->payload[X3D_PAYLOAD_IDX_RETRY] & X3D_RETRY_MASK;
    reg_data->magic_id       = msg->header_magic;

    x3d_read_le_u16(&reg_data->reg_u, msg->payload, X3D_PPED_IDX_PAYLOAD_REG);
    if (msg->payload_length > X3D_PPED_IDX_PAYLOAD_REG_PAYLOAD)
    {
        reg_data->reg_payload_length = msg->payload_length - X3D_PPED_IDX_PAYLOAD_REG_PAYLOAD;
        memcpy(reg_data->reg_payload, &msg->payload[X3D_PPED_IDX_PAYLOAD_REG_PAYLOAD], reg_data->reg_payload_length);
    }
    else
    {
        reg_data->reg_payload_length = 0;
    }

    return reg_data->is_response ? 0 : -6;
}
