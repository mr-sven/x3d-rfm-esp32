#include "x3d.h"

int read_le_u32(uint32_t* res, uint8_t* buffer, int pBuffer)
{
    *res = buffer[pBuffer++];
    *res |= buffer[pBuffer++] << 8;
    *res |= buffer[pBuffer++] << 16;
    *res |= buffer[pBuffer++] << 24;
    return pBuffer;
}

int read_le_u16(uint16_t* res, uint8_t* buffer, int pBuffer)
{
    *res = buffer[pBuffer++];
    *res |= buffer[pBuffer++] << 8;
    return pBuffer;
}

int read_be_i16(int16_t* res, uint8_t* buffer, int pBuffer)
{
    *res = buffer[pBuffer++] << 8;
    *res |= buffer[pBuffer++];
    return pBuffer;
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

int write_le_u16(uint16_t val, uint8_t* buffer, int pBuffer)
{
    buffer[pBuffer++] = val & 0xff;
    buffer[pBuffer++] = (val >> 8) & 0xff;
    return pBuffer;
}

int write_be_i16(int16_t val, uint8_t* buffer, int pBuffer)
{
    buffer[pBuffer++] = (val >> 8) & 0xff;
    buffer[pBuffer++] = val & 0xff;
    return pBuffer;
}

int write_be_u16(uint16_t val, uint8_t* buffer, int pBuffer)
{
    buffer[pBuffer++] = (val >> 8) & 0xff;
    buffer[pBuffer++] = val & 0xff;
    return pBuffer;
}

int write_le_u24(uint32_t val, uint8_t* buffer, int pBuffer)
{
    buffer[pBuffer++] = val & 0xff;
    buffer[pBuffer++] = (val >> 8) & 0xff;
    buffer[pBuffer++] = (val >> 16) & 0xff;
    return pBuffer;
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
    int size = buffer[X3D_IDX_PKT_LEN] - sizeof(uint16_t);
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
    write_be_u16(crc, buffer, size);
}

int x3d_prepare_message_header(uint8_t* buffer, uint8_t* messageNo, uint8_t messageType, uint8_t flags, uint8_t status, uint8_t* extendedHeader, int extendedHeaderLen, uint16_t messageId)
{
    buffer[X3D_IDX_MSG_NO] = *messageNo++;
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

void x3d_set_pairing_data(uint8_t* buffer, int payloadIndex, uint8_t targetSlot, uint16_t pairingPin, uint8_t pairingStatus)
{
    write_le_u16(0xff1f, buffer, payloadIndex + X3D_OFF_REGISTER_ACTION);
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
    buffer[payloadIndex + X3D_OFF_REGISTER_ACTION] = 0xff;
    write_le_u16(targetSlot, buffer, payloadIndex + X3D_OFF_BEACON_TARGET_SLOT_NO);
    int packetSize = write_le_u16(0xffe0, buffer, payloadIndex + X3D_OFF_BEACON_UNKNOWN);
    buffer[X3D_IDX_PKT_LEN] = packetSize + X3D_CRC_SIZE;
}