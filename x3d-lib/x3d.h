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

// delay between X3D messages on RF
#define X3D_MSG_DELAY_MS                    20

// message header length mask and flags
#define X3D_HEADER_LENGTH_MASK              0x1f
#define X3D_HEADER_FLAGS_MASK               0xe0
#define X3D_HEADER_FLAG_NO_RESPONSE         0x20

// second byte of the header extension
#define X3D_HEADER_EXT_NONE                 0x00
#define X3D_HEADER_EXT_EMPTY_BYTE           0x01
// indicate temp data is following
#define X3D_HEADER_EXT_TEMP                 0x08

// third byte of the header extension to indicate the type of temp data
#define X3D_HEADER_EXT_TEMP_ROOM            0x00
#define X3D_HEADER_EXT_TEMP_OUTDOOR         0x01

// 1 byte header len, 3 byte device id, 1 byte network, 1 byte status, 2 byte crosssum
#define X3D_MIN_HEADER_SIZE                 8

// direct index of message type and others
#define X3D_IDX_PKT_LEN                     0
#define X3D_IDX_PKT_ADDR                    1
#define X3D_IDX_MSG_NO                      2
#define X3D_IDX_MSG_TYPE                    3
#define X3D_IDX_HEADER_LEN                  4
#define X3D_IDX_DEVICE_ID                   5
#define X3D_IDX_NETWORK                     8

// message header offsets from network index
#define X3D_OFF_HEADER_STATUS               1
#define X3D_OFF_HEADER_EXT                  2

// message payload offsets from payload index
#define X3D_OFF_RETRANS_SLOT                1
#define X3D_OFF_RETRANS_ACK_SLOT            3

#define X3D_OFF_REGISTER_TARGET             5
#define X3D_OFF_REGISTER_ACTION             7
#define X3D_OFF_REGISTER_HIGH               8
#define X3D_OFF_REGISTER_LOW                9
#define X3D_OFF_REGISTER_ACK                10

#define X3D_OFF_PAIR_UNKNOWN                5
#define X3D_OFF_PAIR_TARGET_SLOT_NO         7
#define X3D_OFF_PAIR_PIN                    9
#define X3D_OFF_PAIR_STATE                  11

#define X3D_OFF_BEACON_UNKNOWN              5
#define X3D_OFF_BEACON_TARGET_SLOT_NO       6
#define X3D_OFF_BEACON_UNKNOWN_2            8

// message payload action types
#define X3D_REGISTER_ACTION_RESET           0x0
#define X3D_REGISTER_ACTION_READ            0x1
#define X3D_REGISTER_ACTION_NONE            0x8
#define X3D_REGISTER_ACTION_WRITE           0x9

#define X3D_CRC_SIZE                        sizeof(uint16_t)

// maximum number of devices per network, due limit of 64 bytes per packet
#define X3D_MAX_NET_DEVICES                 16
#define X3D_MAX_PAYLOAD_DATA_FIELDS         (X3D_MAX_NET_DEVICES)

// Header len byte + header chksum i16
#define X3D_HEADER_CKSUM_DROP_LEN           3

// 0x16 0x11 Flags
#define X3D_FLAG_DEFROST                    0x0200
#define X3D_FLAG_TIMED                      0x0800
#define X3D_FLAG_HEATER_ON                  0x1000
#define X3D_FLAG_HEATER_STOPPED             0x2000
#define X3D_FLAG_STATUS                     0x8000

// 0x16 0x21 Flags
#define X3D_FLAG_WINDOW_OPEN                0x0002
#define X3D_FLAG_NO_TEMP_SENSOR             0x0100
#define X3D_FLAG_BATTERY_LOW                0x1000

// register addresses
#define X3D_REG_ATT_POWER                   0x1151
#define X3D_REG_START_PAIR                  0x1401
#define X3D_REG_ROOM_TEMP                   0x1511
#define X3D_REG_OUTDOOR_TEMP                0x1521
#define X3D_REG_SETPOINT_STATUS             0x1611
#define X3D_REG_ERROR_STATUS                0x1621
#define X3D_REG_SET_MODE_TEMP               0x1631
#define X3D_REG_ON_OFF                      0x1641
#define X3D_REG_MODE_TIME                   0x1661
#define X3D_REG_SETPOINT_DEFROST            0x1681
#define X3D_REG_SETPOINT_NIGHT_DAY          0x1691
#define X3D_REG_ON_TIME_LSB                 0x1910
#define X3D_REG_ON_TIME_MSB                 0x1990

#define X3D_REG_H(R)                        (((R) >> 8) & 0xff)
#define X3D_REG_L(R)                        ((R) & 0xff)


// type of X3D message
typedef enum {
    X3D_MSG_TYPE_SENSOR = 0,
    X3D_MSG_TYPE_STANDARD = 1,
    X3D_MSG_TYPE_PAIRING = 2,
    X3D_MSG_TYPE_BEACON = 3,
} x3d_msg_type_t;

// pairing status
typedef enum {
    X3D_PAIR_STATE_OPEN = 0xe0,
    X3D_PAIR_STATE_PINNED = 0xe5,
} x3d_pair_state_t;

// payload stuct of standard message
typedef struct __attribute__((__packed__)) {
    uint16_t retransmit;
    uint16_t retransmited;
    uint16_t target;
    uint8_t action;
    uint8_t reg_high;
    uint8_t reg_low;
    uint16_t target_ack;
    uint16_t data[X3D_MAX_PAYLOAD_DATA_FIELDS];
} x3d_standard_msg_payload_t;

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

/**
 * @brief sets device write register data for all devices the same data
 *
 * @param buffer pointer to the message buffer
 * @param payloadIndex index of the payload
 * @param targetSlotMask target device slot
 * @param regHigh register high address
 * @param regLow register low address
 * @param value value to write
 */
void x3d_set_register_write_same(uint8_t* buffer, int payloadIndex, uint16_t targetSlotMask, uint8_t regHigh, uint8_t regLow, uint16_t value);

/**
 * @brief sets device write register data for each device separate value
 *
 * @param buffer pointer to the message buffer
 * @param payloadIndex index of the payload
 * @param targetSlotMask target device slot
 * @param regHigh register high address
 * @param regLow register low address
 * @param values pointer to array with values
 */
void x3d_set_register_write(uint8_t* buffer, int payloadIndex, uint16_t targetSlotMask, uint8_t regHigh, uint8_t regLow, uint16_t* values);

/**
 * @brief decrement and set the retry value
 *
 * @param buffer pointer to the message buffer
 * @return uint8_t previous value of retry count
 */
uint8_t x3d_dec_retry(uint8_t* buffer);

/**
 * @brief returns the pairing pin
 *
 * @param buffer pointer to the message buffer
 * @param payloadIndex index of the payload
 * @return uint16_t pairing pin
 */
uint16_t x3d_get_pairing_pin(uint8_t* buffer, int payloadIndex);

/**
 * @brief returns the retransmit acknowledge slot data
 *
 * @param buffer pointer to the message buffer
 * @param payloadIndex index of the payload
 * @return uint16_t acknowledge slot data
 */
uint16_t x3d_get_retrans_ack(uint8_t* buffer, int payloadIndex);

/**
 * @brief increments and encrypts the message id, takes also care of the zero stepover
 *
 * @param pMsgId pointer to the message id
 * @param deviceId 24bit device id
 * @return uint16_t encrypted message id
 */
uint16_t x3d_enc_msg_id(uint16_t *pMsgId, uint32_t deviceId);

/**
 * @brief decrypts the message id
 *
 * @param encMsgId encrypted value
 * @param deviceId 24bit device id
 * @return uint16_t decrypted message id
 */
uint16_t x3d_dec_msg_id(uint16_t encMsgId, uint32_t deviceId);
