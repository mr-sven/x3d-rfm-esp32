#ifndef X3D_H
#define X3D_H

#include <SmingCore.h>

enum class X3DMessageType : uint8_t
{
    Sensor = 0,
    Standard = 1,
    Pairing = 2,
    Beacon = 3,
};

#define X3D_MSG_TYPE_WND       0
#define X3D_MSG_TYPE_STD       1
#define X3D_MSG_TYPE_ASSIGN    2
#define X3D_MSG_TYPE_BEACON    3

#define X3D_HEADER_LENGTH_MASK        0x1f
#define X3D_HEADER_FLAGS_MASK         0xe0
#define X3D_HEADER_FLAG_NO_PAYLOAD    0x20
#define X3D_HEADER_FLAG3_EMPTY_BYTE   0x01
#define X3D_HEADER_FLAG3_TEMP         0x08

struct X3DMessageHeader {
    uint8_t number;
    uint8_t type;
    uint8_t headerLen;
    uint8_t headerFlags;
    uint32_t deviceId;
    uint8_t unknownHeaderFlags1;
    uint8_t unknownHeaderFlags2;
    uint8_t unknownHeaderFlags3;
    uint8_t tempSign;
    uint16_t temperature;
    uint16_t messageId;
    int16_t headerCheck;
};

struct X3DMessagePayload {
    uint8_t retry;
    uint8_t actor;
    uint8_t unknown1;
    uint8_t response;
    uint8_t unknown2;
};

uint8_t parseMessageHeader(uint8_t * buffer, X3DMessageHeader * out);
uint8_t parseMessagePayload(uint8_t * buffer, X3DMessagePayload * out);

#endif