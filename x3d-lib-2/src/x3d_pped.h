/**
 * @file x3d_pped.h
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

typedef struct
{
    uint32_t peer_device_id;
    uint16_t magic_id;
    uint8_t retry_cnt;
    uint16_t reg;
    uint16_t reg_sub;
} x3d_pped_register_req_t;

/**
 * @brief Prepares X3D PPED pairing request.
 *
 * @param buffer pointer to the message buffer
 * @param device_id 24bit device id
 * @param msg_no pointer to number of the message, gets auto incremented
 * @param magic_id pointer magic id,gets auto incremented
 */
void x3d_pped_pairing_req(uint8_t *buffer, uint32_t device_id, uint8_t *msg_no, uint16_t *magic_id);

/**
 * @brief Prepares X3D PPED pairing response.
 *
 * @param buffer pointer to the message buffer
 * @param device_id 24bit device id
 * @param msg_no pointer to number of the message, gets auto incremented
 * @param magic_id pointer magic id,gets auto incremented
 * @param peer_device_id 24bit device id of peer
 */
void x3d_pped_pairing_resp(uint8_t *buffer, uint32_t device_id, uint8_t *msg_no, uint16_t *magic_id, uint32_t peer_device_id);

/**
 * @brief Parses X3D PPED pairing request
 *
 * @param buffer pointer to the message buffer
 * @param peer_device_id pointer to 24bit device id of peer
 * @param retry_cnt pointer retry counter for delay calculation
 * @param magic_id pointer to current magic id that was send
 * @return zero if request is valid
 */
int x3d_pped_parse_pairing_req(uint8_t *buffer, uint32_t *peer_device_id, uint8_t *retry_cnt, uint16_t *magic_id);

/**
 * @brief Parses X3D PPED pairing response
 *
 * @param buffer pointer to the message buffer
 * @param device_id 24bit device id
 * @param peer_device_id pointer to 24bit device id of peer
 * @param retry_cnt pointer retry counter for delay calculation
 * @param magic_id pointer to current magic id that was send
 * @return zero if response is valid
 */
int x3d_pped_parse_pairing_resp(uint8_t *buffer, uint32_t device_id, uint32_t *peer_device_id, uint8_t *retry_cnt, uint16_t *magic_id);

/**
 * @brief Prepares X3D PPED register request.
 *
 * @param buffer pointer to the message buffer
 * @param device_id 24bit device id
 * @param msg_no pointer to number of the message, gets auto incremented
 * @param magic_id pointer magic id,gets auto incremented
 * @param peer_device_id 24bit device id of peer
 * @param reg register to read
 * @param reg_sub sub register to read, if not zero
 */
void x3d_pped_register_req(uint8_t *buffer, uint32_t device_id, uint8_t *msg_no, uint16_t *magic_id, uint32_t peer_device_id, uint16_t reg, uint16_t reg_sub);

/**
 * @brief Prepares X3D PPED register response.
 *
 * @param buffer pointer to the message buffer
 * @param device_id 24bit device id
 * @param msg_no pointer to number of the message, gets auto incremented
 * @param magic_id pointer magic id,gets auto incremented
 * @param peer_device_id 24bit device id of peer
 * @param reg register read
 * @param data_length length of register data
 * @param data register data
 */
void x3d_pped_register_resp(uint8_t *buffer, uint32_t device_id, uint8_t *msg_no, uint16_t *magic_id, uint32_t peer_device_id, uint16_t reg, int data_length, uint8_t *data);

/**
 * @brief Parses X3D PPED register request
 *
 * @param buffer pointer to the message buffer
 * @param device_id 24bit device id
 * @param reg_req pointer to request struct
 * @return zero if request is valid
 */
int x3d_pped_parse_register_req(uint8_t *buffer, uint32_t device_id, x3d_pped_register_req_t *reg_req);
