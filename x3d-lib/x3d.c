/**
 * @file x3d.cpp
 * @author Sven Fabricius (sven.fabricius@livediesel.de)
 * @brief
 * @version 0.1
 * @date 2023-08-06
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifdef CONFIG_IDF_TARGET
#include "esp_crc.h"
#endif

#include "x3d.h"

static inline int read_le_u32(uint32_t* res, uint8_t* buffer, int pBuffer)
{
    *res = buffer[pBuffer++];
    *res |= buffer[pBuffer++] << 8;
    *res |= buffer[pBuffer++] << 16;
    *res |= buffer[pBuffer++] << 24;
    return pBuffer;
}

static inline int read_le_u16(uint16_t* res, uint8_t* buffer, int pBuffer)
{
    *res = buffer[pBuffer++];
    *res |= buffer[pBuffer++] << 8;
    return pBuffer;
}

static inline int read_be_i16(int16_t* res, uint8_t* buffer, int pBuffer)
{
    *res = buffer[pBuffer++] << 8;
    *res |= buffer[pBuffer++];
    return pBuffer;
}


static inline int write_le_u16(uint16_t val, uint8_t* buffer, int pBuffer)
{
    buffer[pBuffer++] = val & 0xff;
    buffer[pBuffer++] = (val >> 8) & 0xff;
    return pBuffer;
}

static inline int write_be_i16(int16_t val, uint8_t* buffer, int pBuffer)
{
    buffer[pBuffer++] = (val >> 8) & 0xff;
    buffer[pBuffer++] = val & 0xff;
    return pBuffer;
}

static inline int write_be_u16(uint16_t val, uint8_t* buffer, int pBuffer)
{
    buffer[pBuffer++] = (val >> 8) & 0xff;
    buffer[pBuffer++] = val & 0xff;
    return pBuffer;
}

static inline int write_le_u24(uint32_t val, uint8_t* buffer, int pBuffer)
{
    buffer[pBuffer++] = val & 0xff;
    buffer[pBuffer++] = (val >> 8) & 0xff;
    buffer[pBuffer++] = (val >> 16) & 0xff;
    return pBuffer;
}


static inline int get_highest_bit(uint16_t value)
{
    return sizeof(unsigned int) * 8 - __builtin_clz(value) - 1;
}

static uint8_t sbox[] = {0x1, 0x0, 0xC, 0x8, 0xA, 0x9, 0xE, 0x7, 0x3, 0x5, 0x4, 0xB, 0x2, 0xF, 0x6, 0xD};
uint16_t apply_sbox(uint16_t value, uint8_t count)
{
	return (value & ((0xf << count) ^ 0xffff)) | (sbox[(value >> count) & 0xf] << count);
}

int16_t calc_header_check(uint8_t* buffer, int headerLen)
{
    int16_t res = 0;
    for (int i = 0; i < headerLen - X3D_HEADER_CKSUM_DROP_LEN; i++)
    {
        res -= buffer[X3D_IDX_DEVICE_ID + i];
    }
    return res;
}

void set_slot(uint8_t* buffer, int index, uint8_t slot)
{
    if (slot > 7)
    {
        index++;
        slot -= 7;
    }

    uint8_t mask = 1 << slot;
    buffer[index] |= mask;
}

void set_retrans_slot(uint8_t* buffer, int payloadIndex, uint8_t replyCnt, uint8_t slot)
{
    set_slot(buffer, payloadIndex + X3D_OFF_RETRANS_ACK_SLOT, slot);
    buffer[payloadIndex] = (replyCnt << 4) | (slot & 0x0f);
}

uint8_t is_retrans_set(uint8_t* buffer, int payloadIndex, uint8_t slot)
{
    if (slot > 7)
    {
        payloadIndex++;
        slot -= 7;
    }
    return buffer[payloadIndex + X3D_OFF_RETRANS_SLOT] & (1 << slot);
}

void x3d_init_message(uint8_t* buffer, uint32_t deviceId, uint8_t network)
{
    buffer[X3D_IDX_PKT_LEN] = 0; // packed len, updated later
    buffer[X3D_IDX_PKT_ADDR] = 0xff;
    buffer[X3D_IDX_MSG_NO] = 0;
    buffer[X3D_IDX_MSG_TYPE] = 0;
    buffer[X3D_IDX_HEADER_LEN] = 0; // header len, updated later
    write_le_u24(deviceId, buffer, X3D_IDX_DEVICE_ID);
    buffer[X3D_IDX_NETWORK] = network;
}

void x3d_set_crc(uint8_t* buffer)
{
    int size = buffer[X3D_IDX_PKT_LEN] - X3D_CRC_SIZE;
#ifdef CONFIG_IDF_TARGET
    uint16_t crc = ~esp_crc16_be(~0x0000, buffer, size);
#else
    uint16_t crc = 0;
    for (int i = 0; i < size; i++)
    {
        crc = crc ^ ((uint16_t)buffer[i] << 8);
        for (int j = 0; j < 8; j++)
        {
            if (crc & 0x8000)
            {
                crc = (crc << 1) ^ 0x1021;
            }
            else
            {
                crc <<= 1;
            }
        }
    }
#endif
    write_be_u16(crc, buffer, size);
}

int x3d_prepare_message_header(uint8_t* buffer, uint8_t* messageNo, x3d_msg_type_t messageType, uint8_t flags, uint8_t status, uint8_t* extendedHeader, int extendedHeaderLen, uint16_t messageId)
{
    buffer[X3D_IDX_MSG_NO] = (*messageNo)++;
    buffer[X3D_IDX_MSG_TYPE] = messageType;
    buffer[X3D_IDX_NETWORK + X3D_OFF_HEADER_STATUS] = status;

    for (int i = 0; i < extendedHeaderLen; i++)
    {
        buffer[X3D_IDX_NETWORK + X3D_OFF_HEADER_EXT + i] = extendedHeader[i];
    }
    int ckSumIndex = X3D_IDX_NETWORK + X3D_OFF_HEADER_EXT + extendedHeaderLen;
    uint8_t headerLength = ((extendedHeaderLen + X3D_MIN_HEADER_SIZE) & X3D_HEADER_LENGTH_MASK);
    if (messageId != 0)
    {
        ckSumIndex = write_le_u16(messageId, buffer, X3D_IDX_NETWORK + X3D_OFF_HEADER_EXT + extendedHeaderLen);
        headerLength += sizeof(uint16_t);
    }
    buffer[X3D_IDX_HEADER_LEN] = (flags & X3D_HEADER_FLAGS_MASK) | headerLength;

    int16_t ckSum = calc_header_check(buffer, headerLength);
    int packetSize = write_be_i16(ckSum, buffer, ckSumIndex);
    buffer[X3D_IDX_PKT_LEN] = packetSize + X3D_CRC_SIZE;
    return packetSize;
}

void x3d_set_message_retrans(uint8_t* buffer, int payloadIndex, uint8_t replyCnt, uint16_t slotMask)
{
    buffer[payloadIndex] = 0x0f & replyCnt;
    write_le_u16(slotMask, buffer, payloadIndex + X3D_OFF_RETRANS_SLOT);
    write_le_u16(0, buffer, payloadIndex + X3D_OFF_RETRANS_ACK_SLOT);
}

void x3d_set_pairing_data(uint8_t* buffer, int payloadIndex, uint8_t targetSlot, uint16_t pairingPin, x3d_pair_state_t pairingStatus)
{
    write_le_u16(0xff1f, buffer, payloadIndex + X3D_OFF_PAIR_UNKNOWN);
    // add unknown high nibble flag
    if (targetSlot > 1 && targetSlot & 0x01)
    {
        targetSlot |= 0x10;
    }

    write_le_u16(targetSlot, buffer, payloadIndex + X3D_OFF_PAIR_TARGET_SLOT_NO);
    write_le_u16(pairingPin, buffer, payloadIndex + X3D_OFF_PAIR_PIN);
    int packetSize = write_le_u16(pairingStatus, buffer, payloadIndex + X3D_OFF_PAIR_STATE);
    buffer[X3D_IDX_PKT_LEN] = packetSize + X3D_CRC_SIZE;
}

void x3d_set_beacon_data(uint8_t* buffer, int payloadIndex, uint8_t targetSlot)
{
    buffer[payloadIndex + X3D_OFF_BEACON_UNKNOWN] = 0xff;
    write_le_u16(targetSlot, buffer, payloadIndex + X3D_OFF_BEACON_TARGET_SLOT_NO);
    int packetSize = write_le_u16(0xffe0, buffer, payloadIndex + X3D_OFF_BEACON_UNKNOWN_2);
    buffer[X3D_IDX_PKT_LEN] = packetSize + X3D_CRC_SIZE;
}

int set_register_and_action(uint8_t* buffer, int payloadIndex, uint16_t targetSlotMask, uint8_t action, uint8_t regHigh, uint8_t regLow)
{
    write_le_u16(targetSlotMask, buffer, payloadIndex + X3D_OFF_REGISTER_TARGET);
    buffer[payloadIndex + X3D_OFF_REGISTER_ACTION] = action;
    buffer[payloadIndex + X3D_OFF_REGISTER_HIGH] = regHigh;
    buffer[payloadIndex + X3D_OFF_REGISTER_LOW] = regLow;
    return write_le_u16(0x0000, buffer, payloadIndex + X3D_OFF_REGISTER_ACK);
}

void x3d_set_register_read(uint8_t* buffer, int payloadIndex, uint16_t targetSlotMask, uint8_t regHigh, uint8_t regLow)
{
    int dataSlotCount = get_highest_bit(targetSlotMask);
    uint8_t action = ((dataSlotCount << 4) & 0xf0) | X3D_REGISTER_ACTION_READ;
    int dataIdx = set_register_and_action(buffer, payloadIndex, targetSlotMask, action, regHigh, regLow);
    for (int i = 0; i <= dataSlotCount; i++)
    {
        dataIdx = write_le_u16(0x0000, buffer, dataIdx);
    }
    buffer[X3D_IDX_PKT_LEN] = dataIdx + X3D_CRC_SIZE;
}

void x3d_set_unpair_device(uint8_t* buffer, int payloadIndex, uint16_t targetSlotMask)
{
    int dataIdx = set_register_and_action(buffer, payloadIndex, targetSlotMask, X3D_REGISTER_ACTION_RESET, 0xe0, 0x00);
    buffer[X3D_IDX_PKT_LEN] = dataIdx + X3D_CRC_SIZE;
}

void x3d_set_ping_device(uint8_t* buffer, int payloadIndex, uint16_t targetSlotMask)
{
    int dataIdx = set_register_and_action(buffer, payloadIndex, targetSlotMask, X3D_REGISTER_ACTION_NONE, 0x00, 0x00);
    buffer[X3D_IDX_PKT_LEN] = dataIdx + X3D_CRC_SIZE;
}

void x3d_set_register_write_same(uint8_t* buffer, int payloadIndex, uint16_t targetSlotMask, uint8_t regHigh, uint8_t regLow, uint16_t value)
{
    int dataSlotCount = get_highest_bit(targetSlotMask);
    uint8_t action = ((dataSlotCount << 4) & 0xf0) | X3D_REGISTER_ACTION_WRITE;
    int dataIdx = set_register_and_action(buffer, payloadIndex, targetSlotMask, action, regHigh, regLow);
    for (int i = 0; i <= dataSlotCount; i++)
    {
        if ((1 << i) & targetSlotMask)
        {
            dataIdx = write_le_u16(value, buffer, dataIdx);
        }
        else
        {
            dataIdx = write_le_u16(0x0000, buffer, dataIdx);
        }
    }
    buffer[X3D_IDX_PKT_LEN] = dataIdx + X3D_CRC_SIZE;
}

void x3d_set_register_write(uint8_t* buffer, int payloadIndex, uint16_t targetSlotMask, uint8_t regHigh, uint8_t regLow, uint16_t* values)
{
    int dataSlotCount = get_highest_bit(targetSlotMask);
    uint8_t action = ((dataSlotCount << 4) & 0xf0) | X3D_REGISTER_ACTION_WRITE;
    int dataIdx = set_register_and_action(buffer, payloadIndex, targetSlotMask, action, regHigh, regLow);
    for (int i = 0; i <= dataSlotCount; i++)
    {
        if ((1 << i) & targetSlotMask)
        {
            dataIdx = write_le_u16(values[i], buffer, dataIdx);
        }
        else
        {
            dataIdx = write_le_u16(0x0000, buffer, dataIdx);
        }
    }
    buffer[X3D_IDX_PKT_LEN] = dataIdx + X3D_CRC_SIZE;
}

uint8_t x3d_dec_retry(uint8_t* buffer)
{
    uint8_t retryIdx = (buffer[X3D_IDX_HEADER_LEN] & X3D_HEADER_LENGTH_MASK) + X3D_IDX_HEADER_LEN;
    uint8_t currentRetry = buffer[retryIdx];
    if (currentRetry > 0)
    {
        buffer[retryIdx] = currentRetry - 1;
    }
    return currentRetry;
}

uint16_t x3d_get_pairing_pin(uint8_t* buffer, int payloadIndex)
{
    uint16_t pin;
    read_le_u16(&pin, buffer, payloadIndex + X3D_OFF_PAIR_PIN);
    return pin;
}

uint16_t x3d_get_retrans_ack(uint8_t* buffer, int payloadIndex)
{
    uint16_t ack;
    read_le_u16(&ack, buffer, payloadIndex + X3D_OFF_RETRANS_ACK_SLOT);
    return ack;
}

uint16_t x3d_enc_msg_id(uint16_t *msg_id_p, uint32_t deviceId)
{
    (*msg_id_p)++;
    if (*msg_id_p == 0)
    {
        *msg_id_p = 1;
    }

    uint16_t result = *msg_id_p;
    uint16_t xor_key = ((deviceId & 0xff) ^ ((deviceId >> 16) & 0xff)) | (deviceId & 0xff00);
    for (int i = 0; i < 32; i++)
    {
        result = apply_sbox(result, i % 13) ^ xor_key;
    }

    return result;
}

uint16_t x3d_dec_msg_id(uint16_t enc_msg_id, uint32_t deviceId)
{
    uint16_t xor_key = ((deviceId & 0xff) ^ ((deviceId >> 16) & 0xff)) | (deviceId & 0xff00);
    for (int i = 31; i >= 0; i--)
    {
        enc_msg_id = apply_sbox(enc_msg_id ^ xor_key, i % 13);
    }
    return enc_msg_id;
}