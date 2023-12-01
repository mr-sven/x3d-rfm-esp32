/**
 * @file x3d.h
 * @author Sven Fabricius (sven.fabricius@livediesel.de)
 * @brief
 * @version 0.1
 * @date 2023-08-06
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <stdint.h>

#define X3D_MSG_DELAY_MS                20

#define X3D_HEADER_LENGTH_MASK          0x1f
#define X3D_HEADER_FLAGS_MASK           0xe0
#define X3D_HEADER_FLAG_NO_RESPONSE     0x20
#define X3D_HEADER_EXT_EMPTY_BYTE       0x01
#define X3D_HEADER_EXT_TEMP             0x08

// 1 byte header len, 3 byte device id, 1 byte network, 1 byte status, 2 byte crosssum
#define X3D_MIN_HEADER_SIZE             8

#define X3D_IDX_PKT_LEN                 0
#define X3D_IDX_PKT_ADDR                1
#define X3D_IDX_MSG_NO                  2
#define X3D_IDX_MSG_TYPE                3
#define X3D_IDX_HEADER_LEN              4
#define X3D_IDX_DEVICE_ID               5
#define X3D_IDX_NETWORK                 8

#define X3D_OFF_HEADER_STATUS           1
#define X3D_OFF_HEADER_EXT              2

#define X3D_OFF_RETRANS_SLOT            1
#define X3D_OFF_RETRANS_ACK_SLOT        3

#define X3D_OFF_REGISTER_TARGET         5
#define X3D_OFF_REGISTER_ACTION         7
#define X3D_OFF_REGISTER_HIGH           8
#define X3D_OFF_REGISTER_LOW            9
#define X3D_OFF_REGISTER_ACK            10

#define X3D_OFF_PAIR_UNKNOWN            5
#define X3D_OFF_PAIR_TARGET_SLOT_NO     7
#define X3D_OFF_PAIR_PIN                9
#define X3D_OFF_PAIR_STATE              11

#define X3D_OFF_BEACON_UNKNOWN          5
#define X3D_OFF_BEACON_TARGET_SLOT_NO   6
#define X3D_OFF_BEACON_UNKNOWN_2        8

#define X3D_REGISTER_ACTION_RESET       0x0
#define X3D_REGISTER_ACTION_READ        0x1
#define X3D_REGISTER_ACTION_NONE        0x8
#define X3D_REGISTER_ACTION_WRITE       0x9

#define X3D_CRC_SIZE                    sizeof(uint16_t)

// Header len byte + header chksum i16
#define X3D_HEADER_CKSUM_DROP_LEN       3

typedef enum
{
    X3D_MSG_TYPE_SENSOR = 0,
    X3D_MSG_TYPE_STANDARD = 1,
    X3D_MSG_TYPE_PAIRING = 2,
    X3D_MSG_TYPE_BEACON = 3,
} x3d_msg_type_t;

typedef enum
{
    X3D_PAIR_STATE_OPEN = 0xe0,
    X3D_PAIR_STATE_PINNED = 0xe5,
} x3d_pair_state_t;

/**
 * @brief Initialize the the output message buffer. On reusing the buffer it is enought to execute once.
 *
 * @param buffer pointer to the message buffer
 * @param deviceId 24bit device id
 * @param network network number
 */
void x3d_init_message(uint8_t* buffer, uint32_t deviceId, uint8_t network);

/**
 * @brief Prepares the message header.
 *
 * @param buffer pointer to the message buffer
 * @param messageNo number of the message
 * @param messageType type of the message
 * @param flags header flags
 * @param status header status
 * @param extendedHeader pointer to extended header buffer
 * @param extendedHeaderLen size of the extended header
 * @param messageId optional message id, if zero the message id is not added to header
 * @return int returns the index of the end of the header, so the start of the payload.
 */
int x3d_prepare_message_header(uint8_t* buffer, uint8_t* messageNo, x3d_msg_type_t messageType, uint8_t flags, uint8_t status, uint8_t* extendedHeader, int extendedHeaderLen, uint16_t messageId);

/**
 * @brief Sets the retransmit slots and reply count.
 *
 * @param buffer pointer to the message buffer
 * @param payloadIndex index of the payload
 * @param replyCnt current reply count
 * @param slotMask retransmit bitfield
 */
void x3d_set_message_retrans(uint8_t* buffer, int payloadIndex, uint8_t replyCnt, uint16_t slotMask);

/**
 * @brief Calculates the CRC sum and updates the value. Call before send.
 *
 * @param buffer pointer to the message buffer
 */
void x3d_set_crc(uint8_t* buffer);

/**
 * @brief Set pairing message data
 *
 * @param buffer pointer to the message buffer
 * @param payloadIndex index of the payload
 * @param targetSlot target slot number of the new device
 * @param pairingPin device pin of the new device
 * @param pairingStatus pair open for new or pin if pointing a responding device via pin
 */
void x3d_set_pairing_data(uint8_t* buffer, int payloadIndex, uint8_t targetSlot, uint16_t pairingPin, x3d_pair_state_t pairingStatus);

/**
 * @brief Sets beacon message data
 *
 * @param buffer pointer to the message buffer
 * @param payloadIndex index of the payload
 * @param targetSlot target slot number of the new device
 */
void x3d_set_beacon_data(uint8_t* buffer, int payloadIndex, uint8_t targetSlot);

/**
 * @brief sets register read data
 *
 * @param buffer pointer to the message buffer
 * @param payloadIndex index of the payload
 * @param targetSlotMask target device slot
 * @param regHigh high number of the register
 * @param regLow low number of the register
 */
void x3d_set_register_read(uint8_t* buffer, int payloadIndex, uint16_t targetSlotMask, uint8_t regHigh, uint8_t regLow);

/**
 * @brief sets device unpair data
 *
 * @param buffer pointer to the message buffer
 * @param payloadIndex index of the payload
 * @param targetSlotMask target device slot
 */
void x3d_set_unpair_device(uint8_t* buffer, int payloadIndex, uint16_t targetSlotMask);

/**
 * @brief sets device ping data
 *
 * @param buffer pointer to the message buffer
 * @param payloadIndex index of the payload
 * @param targetSlotMask target device slot
 */
void x3d_set_ping_device(uint8_t* buffer, int payloadIndex, uint16_t targetSlotMask);

void x3d_set_register_write_same(uint8_t* buffer, int payloadIndex, uint16_t targetSlotMask, uint8_t regHigh, uint8_t regLow, uint16_t value);
void x3d_set_register_write(uint8_t* buffer, int payloadIndex, uint16_t targetSlotMask, uint8_t regHigh, uint8_t regLow, uint16_t* values);

uint8_t x3d_dec_retry(uint8_t* buffer);
uint16_t x3d_get_pairing_pin(uint8_t* buffer, int payloadIndex);
uint16_t x3d_get_retrans_ack(uint8_t* buffer, int payloadIndex);
uint16_t x3d_enc_msg_id(uint16_t *msg_id_p, uint32_t deviceId);
uint16_t x3d_dec_msg_id(uint16_t enc_msg_id, uint32_t deviceId);
