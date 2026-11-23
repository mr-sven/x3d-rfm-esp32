/**
 * @file x3d.c
 * @author Sven Fabricius (sven.fabricius@livediesel.de)
 * @brief
 * @version 2.0
 * @date 2026-11-19
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#ifdef CONFIG_IDF_TARGET
#include "esp_crc.h"
#endif

#include "x3d.h"

// Header len byte + header chksum i16
#define X3D_HEADER_CKSUM_DROP_LEN 3
#define X3D_HEADER_LENGTH_MASK    0x1f

#define header_crc_off(h_len)     (X3D_IDX_HEADER_LEN + h_len - sizeof(uint16_t))
#define header_payload_len(h_len) (h_len - X3D_IDX_DEVICE_ID - sizeof(uint16_t))
#define payload_len(p_len, h_len) (p_len - h_len - X3D_IDX_HEADER_LEN - sizeof(uint16_t))
#define payload_off(h_len)        (X3D_IDX_HEADER_LEN + h_len)

unit_static int16_t calc_header_check(uint8_t *buffer, int header_len)
{
    int16_t res = 0;
    for (int i = 0; i < header_len - X3D_HEADER_CKSUM_DROP_LEN; i++)
    {
        res -= buffer[X3D_IDX_DEVICE_ID + i];
    }
    return res;
}

static uint8_t sbox[] = {0x1, 0x0, 0xC, 0x8, 0xA, 0x9, 0xE, 0x7, 0x3, 0x5, 0x4, 0xB, 0x2, 0xF, 0x6, 0xD};
unit_static uint16_t apply_sbox(uint16_t value, uint8_t count)
{
    return (value & ((0xf << count) ^ 0xffff)) | (sbox[(value >> count) & 0xf] << count);
}

unit_static uint16_t enc_magic_id(uint16_t magic_id, uint32_t device_id)
{
    uint16_t xor_key = ((device_id & 0xff) ^ ((device_id >> 16) & 0xff)) | (device_id & 0xff00);
    for (int i = 0; i < 32; i++)
    {
        magic_id = apply_sbox(magic_id, i % 13) ^ xor_key;
    }
    return magic_id;
}

unit_static uint16_t dec_magic_id(uint16_t magic_id, uint32_t device_id)
{
    uint16_t xor_key = ((device_id & 0xff) ^ ((device_id >> 16) & 0xff)) | (device_id & 0xff00);
    for (int i = 31; i >= 0; i--)
    {
        magic_id = apply_sbox(magic_id ^ xor_key, i % 13);
    }
    return magic_id;
}

unit_static void set_crc(uint8_t *buffer)
{
    int size = buffer[X3D_IDX_PKT_LEN] - sizeof(uint16_t);
#ifdef CONFIG_IDF_TARGET
    uint16_t crc = ~esp_crc16_be(~0x0000, buffer, size);
#else
    uint16_t crc = 0;
    for (int i = 0; i < size; i++)
    {
        crc = crc ^ ((uint16_t)buffer[i] << 8);
        for (int j = 0; j < 8; j++)
        {
            if (crc & 0x8000)
            {
                crc = (crc << 1) ^ 0x1021;
            }
            else
            {
                crc <<= 1;
            }
        }
    }
#endif
    x3d_write_be_u16(crc, buffer, size);
}

x3d_msg_t *x3d_proto_parse_verify_msg(uint8_t *buffer)
{
    if (buffer[0] < X3D_MIN_PKT_SIZE || buffer[0] > X3D_MAX_PKT_SIZE)
    {
        return NULL;
    }

    // check header CRC
    uint8_t header_length = buffer[X3D_IDX_HEADER_LEN] & X3D_HEADER_LENGTH_MASK;
    int16_t head_crc;
    x3d_read_be_i16(&head_crc, buffer, header_crc_off(header_length));
    if (calc_header_check(buffer, header_length) != head_crc)
    {
        return NULL;
    }

    // prepare header
    x3d_msg_t *result = calloc(1, sizeof(x3d_msg_t));
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    memcpy(&result->msg_header, buffer, sizeof(x3d_msg_t));
#else
    int data_p                    = 0;
    result->msg_header.pkt_len    = buffer[data_p++];
    result->msg_header.ff         = buffer[data_p++];
    result->msg_header.msg_no     = buffer[data_p++];
    result->msg_header.msg_type   = buffer[data_p++];
    result->msg_header.head_len_u = buffer[data_p++];
    uint32_t device_id;
    data_p                       = x3d_read_le_u24(&device_id, buffer, data_p);
    result->msg_header.device_id = device_id;
    result->msg_header.net       = buffer[data_p++];
#endif

    // copy header payload
    result->header_payload_length = header_payload_len(result->msg_header.head_len);
    memcpy(result->header_payload, &buffer[X3D_IDX_HEADER_PAYLOAD], result->header_payload_length);

    // status + magic
    if (result->header_payload_length > 2)
    {
        // decrypt magic id
        uint16_t magic_id;
        x3d_read_le_u16(&magic_id, result->header_payload, result->header_payload_length - sizeof(uint16_t));
        result->header_magic = dec_magic_id(magic_id, result->msg_header.device_id);
    }
    else
    {
        result->header_magic = 0;
    }

    // copy data payload
    result->payload_length = payload_len(result->msg_header.pkt_len, result->msg_header.head_len);
    if (result->payload_length > 0)
    {
        memcpy(result->payload, &buffer[payload_off(result->msg_header.head_len)], result->payload_length);
    }

    return result;
}

x3d_msg_t *x3d_proto_init_message(uint8_t msg_type, uint32_t device_id, uint8_t network)
{
    x3d_msg_t *res             = calloc(1, sizeof(x3d_msg_t));
    res->msg_header.pkt_len    = 0;
    res->msg_header.pkt_addr   = 0xff;
    res->msg_header.msg_no     = 0;
    res->msg_header.msg_type   = msg_type;
    res->msg_header.head_len_u = 0;
    res->msg_header.device_id  = device_id;
    res->msg_header.net        = network;
    res->header_payload_length = 0;
    res->payload_length        = 0;
    res->header_magic          = 0;
    return res;
}

void x3d_proto_prepare_message_header(x3d_msg_t *msg, uint8_t *msg_no, uint8_t flags, uint8_t status, uint8_t *header_data, int header_data_len, uint16_t *magic_id)
{
    (*msg_no)++;
    if (*msg_no == 0)
    {
        *msg_no = 1;
    }
    msg->msg_header.msg_no     = *msg_no;
    msg->msg_header.head_flags = flags;
    // minimum for status
    msg->header_payload_length = 1;

    if (magic_id != NULL)
    {
        (*magic_id)++;
        if (*magic_id == 0)
        {
            *magic_id = 1;
        }
        msg->header_magic = *magic_id;

        // header payload len extend with magic id
        msg->header_payload_length += sizeof(uint16_t);
    }

    if (header_data != NULL && header_data_len > 0)
    {
        msg->header_payload_length += header_data_len;
        memcpy(&msg->header_payload[1], header_data, header_data_len);
    }

    msg->header_payload[0] = status;
    // len field + device_id/net + Payload + len field + cksum
    msg->msg_header.head_len = sizeof(uint8_t) + sizeof(uint32_t) + msg->header_payload_length + sizeof(uint16_t);
}

void x3d_proto_write_to_buffer(uint8_t *buffer, x3d_msg_t *msg)
{
    int data_p = 0;

    msg->msg_header.pkt_len = X3D_IDX_HEADER_LEN + msg->msg_header.head_len + msg->payload_length + sizeof(uint16_t);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    memcpy(buffer, &msg->msg_header, sizeof(x3d_msg_header_t));
#else
    buffer[data_p++] = msg->msg_header.pkt_len;
    buffer[data_p++] = msg->msg_header.pkt_addr;
    buffer[data_p++] = msg->msg_header.msg_no;
    buffer[data_p++] = msg->msg_header.msg_type;
    buffer[data_p++] = msg->msg_header.head_len_u;
    data_p           = x3d_write_le_u24(msg->msg_header.device_id, buffer, data_p);
    buffer[data_p++] = msg->msg_header.net;
#endif
    if (msg->header_magic != 0)
    {
        uint16_t magic_id = enc_magic_id(msg->header_magic, msg->msg_header.device_id);
        x3d_write_le_u16(magic_id, msg->header_payload, msg->header_payload_length - sizeof(uint16_t));
    }
    memcpy(&buffer[X3D_IDX_HEADER_PAYLOAD], msg->header_payload, msg->header_payload_length);
    data_p = X3D_IDX_HEADER_PAYLOAD + msg->header_payload_length;

    int16_t header_crc = calc_header_check(buffer, msg->msg_header.head_len);
    data_p             = x3d_write_be_i16(header_crc, buffer, data_p);

    if (msg->payload_length > 0)
    {
        memcpy(&buffer[data_p], msg->payload, msg->payload_length);
    }

    set_crc(buffer);
}

uint8_t x3d_proto_dec_retry(uint8_t *buffer)
{
    uint8_t retryIdx     = (buffer[X3D_IDX_HEADER_LEN] & X3D_HEADER_LENGTH_MASK) + X3D_IDX_HEADER_LEN;
    uint8_t currentRetry = buffer[retryIdx] & X3D_RETRY_MASK;
    if (currentRetry > 0)
    {
        buffer[retryIdx] = (buffer[retryIdx] & X3D_RETRY_MASK_INV) | (currentRetry - 1);
    }
    set_crc(buffer);
    return currentRetry;
}
