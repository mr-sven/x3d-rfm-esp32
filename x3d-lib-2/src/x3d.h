/**
 * @file x3d.h
 * @author Sven Fabricius (sven.fabricius@livediesel.de)
 * @brief
 * @version 2.0
 * @date 2026-11-19
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include "x3d_bytes.h"
#include "x3d_proto.h"
#include "x3d_pped.h"

// delay between X3D messages on RF
#define X3D_MSG_DELAY_MS 20

// direct index of message type and others
#define X3D_IDX_PKT_LEN        0
#define X3D_IDX_PKT_ADDR       1
#define X3D_IDX_MSG_NO         2
#define X3D_IDX_MSG_TYPE       3
#define X3D_IDX_HEADER_LEN     4
#define X3D_IDX_DEVICE_ID      5
#define X3D_IDX_NETWORK        8
#define X3D_IDX_HEADER_PAYLOAD 9

// Header flags
#define X3D_HEADER_FLAG_NONE        0
#define X3D_HEADER_FLAG_NO_RESPONSE 1

#define X3D_RETRY_MASK              0x07
#define X3D_RETRY_MASK_INV          0xf8

#if UNIT_TEST
#define unit_static
#else
#define unit_static static
#endif
