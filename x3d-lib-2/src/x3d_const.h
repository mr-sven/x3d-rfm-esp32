/**
 * @file x3d_const.h
 * @author Sven Fabricius (sven.fabricius@livediesel.de)
 * @brief
 * @version 1.0
 * @date 2026-11-23
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

// delay between X3D messages on RF
#define X3D_MSG_DELAY_MS 20

// len, addr, msg_no, msg_type, head_len, 3 byte device id, network, status, 2 byte header_crc, 2 byte crc
#define X3D_MIN_PKT_SIZE            14
#define X3D_MAX_PKT_SIZE            64

#define X3D_MAX_PAYLOAD_SIZE        (X3D_MAX_PKT_SIZE - X3D_MIN_PKT_SIZE)
#define X3D_MAX_HEADER_PAYLOAD_SIZE (X3D_MAX_PKT_SIZE - X3D_MIN_PKT_SIZE)

// direct index of message type and others
#define X3D_IDX_PKT_LEN        0
#define X3D_IDX_PKT_ADDR       1
#define X3D_IDX_MSG_NO         2
#define X3D_IDX_MSG_TYPE       3
#define X3D_IDX_HEADER_LEN     4
#define X3D_IDX_DEVICE_ID      5
#define X3D_IDX_NETWORK        8
#define X3D_IDX_HEADER_PAYLOAD 9

#define X3D_PAYLOAD_IDX_RETRY  0

// Header flags
#define X3D_HEADER_FLAG_NONE        0
#define X3D_HEADER_FLAG_NO_RESPONSE 1

#define X3D_RETRY_MASK              0x07
#define X3D_RETRY_MASK_INV          0xf8
