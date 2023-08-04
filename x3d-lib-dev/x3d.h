#ifndef X3D_H
#define X3D_H

#include <stdint.h>

#define X3D_MSG_TYPE_SENSOR             0
#define X3D_MSG_TYPE_STD                1
#define X3D_MSG_TYPE_PAIRING            2
#define X3D_MSG_TYPE_BEACON             3

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

#define X3D_OFF_REGISTER_ACTION         5

#define X3D_OFF_PAIR_TARGET_SLOT_NO     7
#define X3D_OFF_PAIR_PIN                9
#define X3D_OFF_PAIR_STATE              11

#define X3D_OFF_BEACON_TARGET_SLOT_NO   6
#define X3D_OFF_BEACON_UNKNOWN          8

#define X3D_PAIR_STATE_OPEN             0xe0
#define X3D_PAIR_STATE_PINNED           0xe5

#define X3D_PAIR_RESULT_RET_PIN         1
#define X3D_PAIR_RESULT_PIN_VALID       2
#define X3D_PAIR_RESULT_RETRANS         3

#define X3D_CRC_SIZE                    sizeof(uint16_t)


// Header len byte + header chksum i16
#define X3D_HEADER_CKSUM_DROP_LEN       3

enum class X3dMessageType : uint8_t
{
    Sensor = 0,
    Standard = 1,
    Pairing = 2,
    Beacon = 3,
};

int read_le_u32(uint32_t* res, uint8_t* buffer, int pBuffer);
int read_le_u16(uint16_t* res, uint8_t* buffer, int pBuffer);
int read_be_i16(int16_t* res, uint8_t* buffer, int pBuffer);

int write_le_u16(uint16_t val, uint8_t* buffer, int pBuffer);
int write_be_i16(int16_t val, uint8_t* buffer, int pBuffer);
int write_be_u16(uint16_t val, uint8_t* buffer, int pBuffer);
int write_le_u24(uint32_t val, uint8_t* buffer, int pBuffer);

int16_t calc_header_check(uint8_t* buffer, int headerLen);

void set_slot(uint8_t* buffer, int index, uint8_t slot);
void set_retrans_slot(uint8_t* buffer, int payloadIndex, uint8_t replyCnt, uint8_t slot);
uint8_t is_retrans_set(uint8_t* buffer, int payloadIndex, uint8_t slot);

void x3d_init_message(uint8_t* buffer, uint32_t deviceId, uint8_t network);
void x3d_set_crc(uint8_t* buffer);

int x3d_prepare_message_header(uint8_t* buffer, uint8_t* messageNo, uint8_t messageType, uint8_t flags, uint8_t status, uint8_t* extendedHeader, int extendedHeaderLen, uint16_t messageId);
void x3d_set_message_retrans(uint8_t* buffer, int payloadIndex, uint8_t replyCnt, uint16_t slotMask);

void x3d_set_pairing_data(uint8_t* buffer, int payloadIndex, uint8_t targetSlot, uint16_t pairingPin, uint8_t pairingStatus);
void x3d_set_beacon_data(uint8_t* buffer, int payloadIndex, uint8_t targetSlot);
#endif