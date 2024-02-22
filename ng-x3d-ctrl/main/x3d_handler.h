/**
 * @file x3d_handler.h
 * @author Sven Fabricius (sven.fabricius@livediesel.de)
 * @brief
 * @version 0.1
 * @date 2023-12-06
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "x3d.h"

/// @brief Task processing data for pairing
typedef struct {
    uint8_t network;
    uint16_t transfer;
} x3d_pairing_data_t;

/// @brief Task processing data for unpairing
typedef struct {
    uint8_t network;
    uint16_t transfer;
    uint16_t target;
} x3d_unpairing_data_t;

/// @brief Task processing data for register read
typedef struct {
    uint8_t network;
    uint16_t transfer;
    uint16_t target;
    uint8_t register_high;
    uint8_t register_low;
} x3d_read_data_t;

/// @brief Task processing data for register write
typedef struct {
    uint8_t network;
    uint16_t transfer;
    uint16_t target;
    uint8_t register_high;
    uint8_t register_low;
    uint16_t values[X3D_MAX_PAYLOAD_DATA_FIELDS];
} x3d_write_data_t;

/// @brief Task processing data for temp update
typedef struct {
    uint8_t network;
    uint16_t transfer;
    uint16_t target;
    uint8_t outdoor;
    uint16_t temp;
} x3d_temp_data_t;

/**
 * @brief Sets the device id for x3d processing
 *
 * @param device_id the device id
 */
void x3d_set_device_id(uint32_t device_id);

/**
 * @brief Execute pairing process
 *
 * @param data pointer to x3d_pairing_data_t
 * @return int, -1 = error, 0..15 = target device number
 */
int x3d_pairing_proc(x3d_pairing_data_t *data);

/**
 * @brief Execute unpairing process
 *
 * @param data pointer to x3d_unpairing_data_t
 */
void x3d_unpairing_proc(x3d_unpairing_data_t *data);

/**
 * @brief Execute the register read process
 *
 * @param data pointer to x3d_read_data_t
 * @return x3d_standard_msg_payload_t * do not free the pointer it's pointing to the static x3d processing buffer
 */
x3d_standard_msg_payload_t * x3d_reading_proc(x3d_read_data_t *data);

/**
 * @brief Execute the register write process
 *
 * @param data pointer to x3d_write_data_t
 * @return x3d_standard_msg_payload_t * do not free the pointer it's pointing to the static x3d processing buffer
 */
x3d_standard_msg_payload_t * x3d_writing_proc(x3d_write_data_t *data);

/**
 * @brief Execute the temp update process
 *
 * @param data pointer to x3d_temp_data_t
 */
void x3d_temp_proc(x3d_temp_data_t *data);