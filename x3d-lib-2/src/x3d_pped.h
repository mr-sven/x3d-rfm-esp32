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
 * @brief Verifies X3D PPED pairing request
 *
 * @param buffer pointer to the message buffer
 * @param device_id pointer to 24bit device id of peer
 * @param retry_cnt pointer retry counter for delay calculation
 * @return zero if request is valid
 */
int x3d_pped_is_valid_pairing_req(uint8_t *buffer, uint32_t *device_id, uint8_t *retry_cnt);

/**
 * @brief Verifies X3D PPED pairing response
 *
 * @param buffer pointer to the message buffer
 * @param device_id 24bit device id
 * @param peer_device_id pointer to 24bit device id of peer
 * @param retry_cnt pointer retry counter for delay calculation
 * @return zero if response is valid
 */
int x3d_pped_is_valid_pairing_resp(uint8_t *buffer, uint32_t device_id, uint32_t *peer_device_id, uint8_t *retry_cnt);
