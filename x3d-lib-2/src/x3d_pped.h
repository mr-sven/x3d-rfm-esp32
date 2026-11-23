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
#include <stdbool.h>

#include "x3d_types.h"

#define X3D_PPED_IDX_PAYLOAD_TARGET_DEVICE_ID 2
#define X3D_PPED_IDX_PAYLOAD_REG              5
#define X3D_PPED_IDX_PAYLOAD_REG_PAYLOAD      7

#define X3D_MAX_PPED_REG_PAYLOAD_SIZE         (X3D_MAX_PAYLOAD_SIZE - X3D_PPED_IDX_PAYLOAD_REG_PAYLOAD)

typedef struct
{
    uint32_t peer_device_id; // device id of peer device
    uint16_t magic_id;       // current magic id read on parse
    uint8_t retry_cnt;       // current retry counter read on parse
    union
    {
        struct
        {
            uint16_t reg : 14;    // LSB data register addess
            bool is_write : 1;    // msb write flag
            bool is_response : 1; // msb response flag
        };
        uint16_t reg_u; // register union
    };
    int reg_payload_length;                             // length of payload
    uint8_t reg_payload[X3D_MAX_PPED_REG_PAYLOAD_SIZE]; // reg payload data
} x3d_pped_register_t;

typedef struct
{
    union
    {
        struct
        {
            uint8_t month : 4;
            uint8_t month_f : 4;
        };
        uint8_t month_u; // register union
    };
    uint8_t day;
    uint8_t year;
    uint32_t heating;
    uint32_t cooling;
    uint32_t hot_water;
    uint32_t outlets;
    uint32_t other;
} x3d_pped_month_consumtion_detail_t;

/**
 * @brief Prepares X3D PPED pairing request.
 *
 * @param buffer pointer to the message buffer
 * @param ctx pointer to x3d_context contain device id, msg number and magic id
 */
void x3d_pped_pairing_req(uint8_t *buffer, x3d_context_t *ctx);

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
 * @brief Prepares X3D PPED pairing response.
 *
 * @param buffer pointer to the message buffer
 * @param ctx pointer to x3d_context contain device id, msg number and magic id
 * @param peer_device_id 24bit device id of peer
 */
void x3d_pped_pairing_resp(uint8_t *buffer, x3d_context_t *ctx, uint32_t peer_device_id);

/**
 * @brief Parses X3D PPED pairing response
 *
 * @param buffer pointer to the message buffer
 * @param ctx pointer to x3d_context contain device id, msg number and magic id
 * @param peer_device_id pointer to 24bit device id of peer
 * @param retry_cnt pointer retry counter for delay calculation
 * @param magic_id pointer to current magic id that was send
 * @return zero if response is valid
 */
int x3d_pped_parse_pairing_resp(uint8_t *buffer, x3d_context_t *ctx, uint32_t *peer_device_id, uint8_t *retry_cnt, uint16_t *magic_id);

/**
 * @brief Prepares X3D PPED register request.
 *
 * @param buffer pointer to the message buffer
 * @param ctx pointer to x3d_context contain device id, msg number and magic id
 * @param reg_data pointer to request struct
 */
void x3d_pped_register_req(uint8_t *buffer, x3d_context_t *ctx, x3d_pped_register_t *reg_data);

/**
 * @brief Parses X3D PPED register request
 *
 * @param buffer pointer to the message buffer
 * @param ctx pointer to x3d_context contain device id, msg number and magic id
 * @param reg_data pointer to request struct
 * @return zero if request is valid
 */
int x3d_pped_parse_register_req(uint8_t *buffer, x3d_context_t *ctx, x3d_pped_register_t *reg_data);

/**
 * @brief Prepares X3D PPED register response.
 *
 * @param buffer pointer to the message buffer
 * @param ctx pointer to x3d_context contain device id, msg number and magic id
 * @param reg_data pointer to response struct
 */
void x3d_pped_register_resp(uint8_t *buffer, x3d_context_t *ctx, x3d_pped_register_t *reg_data);

/**
 * @brief Parses X3D PPED register response
 *
 * @param buffer pointer to the message buffer
 * @param ctx pointer to x3d_context contain device id, msg number and magic id
 * @param reg_data pointer to response struct
 * @return zero if request is valid
 */
int x3d_pped_parse_register_resp(uint8_t *buffer, x3d_context_t *ctx, x3d_pped_register_t *reg_data);
