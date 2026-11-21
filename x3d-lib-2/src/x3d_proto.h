/**
 * @file x3d_proto.h
 * @author Sven Fabricius (sven.fabricius@livediesel.de)
 * @brief
 * @version 1.0
 * @date 2026-11-20
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include "x3d_types.h"

/**
 * @brief Parses buffer to x3d message struct
 *
 * @param buffer pointer to the message buffer
 * @return pointer to message struct or NULL if header CRC incorrect
 */
x3d_msg_t *x3d_proto_parse_verify_msg(uint8_t *buffer);

/**
 * @brief Initialize the the output message buffer. On reusing the buffer it is enought to execute once.
 *
 * @param msg_type type of message
 * @param device_id 24bit device id
 * @param network network number
 * @return pointer to message struct
 */
x3d_msg_t *x3d_proto_init_message(uint8_t msg_type, uint32_t device_id, uint8_t network);

/**
 * @brief Prepares the message header.
 *
 * @param msg pointer to the message struct
 * @param msg_no pointer to number of the message, gets auto incremented
 * @param flags header flags
 * @param status header status
 * @param header_data pointer to extended header buffer
 * @param header_data_len size of the extended header
 * @param magic_id optional magic id, if NULL the message id is not added to header else it is auto incremented
 */
void x3d_proto_prepare_message_header(x3d_msg_t *msg, uint8_t *msg_no, uint8_t flags, uint8_t status, uint8_t *header_data, int header_data_len, uint16_t *magic_id);

/**
 * @brief Transfer message struct to buffer
 *
 * @param buffer pointer to the message buffer
 * @param msg pointer to the message struct
 */
void x3d_proto_write_to_buffer(uint8_t *buffer, x3d_msg_t *msg);

/**
 * @brief decrement and set the retry value
 *
 * @param buffer pointer to the message buffer
 * @return previous value of retry count
 */
uint8_t x3d_proto_dec_retry(uint8_t *buffer);
