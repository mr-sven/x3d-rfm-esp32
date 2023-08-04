#include <iostream>
#include <iomanip>
#include "x3d.h"

int main()
{
    std::cout << "Hello World!\n";
    uint8_t buffer[64];

    uint32_t deviceId = 0x123456;
    uint8_t msgNo = 1;
    uint16_t msgId = 0x1111;
    int replyCnt = 4;

    x3d_init_message(buffer, deviceId, 0x00);

    /* Pairing messages
    uint8_t extHeader[] = {0x98, 0x00};
    int payloadIndex = x3d_prepare_message_header(buffer, &msgNo, X3D_MSG_TYPE_PAIRING, 0, 0x85, extHeader, sizeof(extHeader), msgId);
    x3d_set_message_retrans(buffer, payloadIndex, replyCnt, 0x0000);

    // x3d_set_pairing_data(buffer, payloadIndex, 0, 0, X3D_PAIR_STATE_OPEN);
    x3d_set_pairing_data(buffer, payloadIndex, 0, 0x1234, X3D_PAIR_STATE_PINNED);
    // */

    /* Beacon message
    uint8_t extHeader[] = {0x98, 0x00};
    int payloadIndex = x3d_prepare_message_header(buffer, &msgNo, X3D_MSG_TYPE_BEACON, 0, 0x05, extHeader, sizeof(extHeader), msgId);
    x3d_set_message_retrans(buffer, payloadIndex, replyCnt, 0x0000);
    x3d_set_beacon_data(buffer, payloadIndex, 0);
    // */

    //* Sensor message
    uint8_t extHeader[] = { 0x82, 0x00, 0x03, 0x08, 0x12, 0x04, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x21 };
    int payloadIndex = x3d_prepare_message_header(buffer, &msgNo, X3D_MSG_TYPE_SENSOR, X3D_HEADER_FLAG_NO_RESPONSE, 0x02, extHeader, sizeof(extHeader), 0);
    x3d_set_crc(buffer);
    // */

    /* Standard message
    uint8_t extHeader[] = {0x98, 0x00};
    int payloadIndex = x3d_prepare_message_header(buffer, &msgNo, X3D_MSG_TYPE_STD, 0, 0x05, extHeader, sizeof(extHeader), msgId);
    x3d_set_message_retrans(buffer, payloadIndex, replyCnt, 0x0000);
    //x3d_set_beacon_data(buffer, payloadIndex, 0);
    // */

    do
    {
        x3d_set_message_retrans(buffer, payloadIndex, replyCnt, 0x0000);
        x3d_set_crc(buffer);

        for (int i = 0; i < 64; i++)
        {
            std::cout << " " << std::hex << std::setfill('0') << std::setw(2) << (int)buffer[i];
        }
        std::cout << std::dec << std::endl;
        replyCnt--;

    } while (replyCnt >= 0);
}