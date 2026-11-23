/**
 * @file x3d_types.h
 * @author Sven Fabricius (sven.fabricius@livediesel.de)
 * @brief
 * @version 1.0
 * @date 2026-11-20
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include <stdint.h>

typedef struct x3d_context_t
{
    uint32_t device_id;
    uint8_t msg_no;
    uint16_t magic_id;
} x3d_context_t;

typedef struct __attribute__((__packed__))
{
    uint8_t pkt_len;
    uint8_t pkt_addr;
    uint8_t msg_no;
    uint8_t msg_type;
    union
    {
        struct
        {
            uint8_t head_len : 5;
            uint8_t head_flags : 3;
        };
        uint8_t head_len_u;
    };

    union
    {
        struct
        {
            uint32_t device_id : 24;
            uint8_t net;
        };
        uint32_t device_id_u;
    };

} x3d_msg_header_t;

typedef struct
{
    x3d_msg_header_t msg_header;
    uint8_t *header_payload;
    int header_payload_length;
    uint16_t header_magic;
    uint8_t *payload;
    int payload_length;
} x3d_msg_t;
