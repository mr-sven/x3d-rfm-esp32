#include "X3D.h"

uint32_t read_le_u32(uint8_t ** buffer)
{
    uint32_t res = **buffer; (*buffer)++;
    res |= **buffer << 8; (*buffer)++;
    res |= **buffer << 16; (*buffer)++;
    res |= **buffer << 24; (*buffer)++;
    return res;
}

uint16_t read_le_u16(uint8_t ** buffer)
{
    uint16_t res = **buffer; (*buffer)++;
    res |= **buffer << 8; (*buffer)++;
    return res;
}

uint16_t read_be_u16(uint8_t ** buffer)
{
    uint16_t res = **buffer << 8; (*buffer)++;
    res |= **buffer; (*buffer)++;
    return res;
}

uint8_t parseMessageHeader(uint8_t * buffer, X3DMessageHeader * out)
{
    uint8_t bytes_read = 14;
    out->number = *buffer++;
    out->type = *buffer++;
    out->headerLen = *buffer & X3D_HEADER_LENGTH_MASK;
    out->headerFlags = *buffer++ & X3D_HEADER_FLAGS_MASK;
    out->deviceId = read_le_u32(&buffer);
    out->unknownHeaderFlags1 = *buffer++;
    out->unknownHeaderFlags2 = *buffer++;
    out->unknownHeaderFlags3 = *buffer++;
    if (out->unknownHeaderFlags3 == X3D_HEADER_FLAG3_EMPTY_BYTE)
    {
        buffer++;
        bytes_read++;
    }
    else if (out->unknownHeaderFlags3 == X3D_HEADER_FLAG3_TEMP)
    {
        out->tempSign = *buffer++;
        out->temperature = read_le_u16(&buffer);
        bytes_read += 3;
    }
    out->messageId = read_le_u16(&buffer);
    out->headerCheck = read_be_u16(&buffer);

    return bytes_read;
}

uint8_t parseMessagePayload(uint8_t * buffer, X3DMessagePayload * out)
{
    uint8_t bytes_read = 5;
    out->retry = *buffer++;
    out->actor = *buffer++;
    out->unknown1 = *buffer++;
    out->response = *buffer++;
    out->unknown2 = *buffer++;
    return bytes_read;
}