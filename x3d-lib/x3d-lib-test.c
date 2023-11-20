#include <stdio.h>
#include "x3d.h"
/*
int process_pairing_message(uint8_t* buffer, uint16_t pairingId, uint8_t * slot, uint8_t replyCnt)
{
    int payloadIndex = (buffer[X3D_IDX_HEADER_LEN] & X3D_HEADER_LENGTH_MASK) + X3D_IDX_HEADER_LEN;
    uint8_t pairingStatus = buffer[payloadIndex + X3D_OFF_PAIR_STATE];
    uint8_t targetSlotNo = buffer[payloadIndex + X3D_OFF_PAIR_TARGET_SLOT_NO] & 0x0f; // drop higher nibble
    uint16_t currentPairingId;
    read_le_u16(&currentPairingId, buffer, payloadIndex + X3D_OFF_PAIR_PIN);

    if (*slot == 0 &&            // unpaired
        pairingId != 0 &&        // pairing pin id
        currentPairingId == 0 && // new pairing request
        pairingStatus == X3D_PAIR_STATE_OPEN)
    {
        write_le_u16(pairingId, buffer, payloadIndex + X3D_OFF_PAIR_PIN);
        set_retrans_slot(buffer, payloadIndex, replyCnt, targetSlotNo);
        return X3D_PAIR_RESULT_RET_PIN;
    }
    else if (*slot == 0 &&        // unpaired
        pairingId != 0 &&         // pairing pin id
        currentPairingId == pairingId && // new pairing request
        pairingStatus == X3D_PAIR_STATE_PINNED)
    {
        write_le_u16(pairingId, buffer, payloadIndex + X3D_OFF_PAIR_PIN);
        set_retrans_slot(buffer, payloadIndex, replyCnt, targetSlotNo);
        *slot = targetSlotNo;
        return X3D_PAIR_RESULT_PIN_VALID;
    }
    else if (pairingId == 0 && is_retrans_set(buffer, payloadIndex, *slot)) // pairing pin zero, so is paired
    {
        set_retrans_slot(buffer, payloadIndex, replyCnt, *slot);
        return X3D_PAIR_RESULT_RETRANS;
    }

    return -1;
}*/
/*
void hexToBytes(const std::string& data, uint8_t* buffer)
{
    for (int i = 0; i < data.length(); i += 2)
    {
        std::string byteString = data.substr(i, 2);
        buffer[i / 2] = (uint8_t)strtol(byteString.c_str(), NULL, 16);
    }
}
*/

void print_buffer(uint8_t* buffer)
{
    int bufsize = buffer[0];
    for (int i = 0; i < bufsize; i++)
    {
        printf(" %02x", buffer[i]);
    }
    printf("\n");
}

int main()
{
    printf("Hello World!\n");
    uint8_t buffer[64];

    uint32_t deviceId = 0x123456;
    uint8_t msgNo = 1;
    uint16_t msgId = 0x1111;
    int replyCnt = 4;
    uint16_t transferSlotMask = 0x0000;
    uint16_t targetSlotMask = 0x0001;


    x3d_init_message(buffer, deviceId, 0x00);

    /* Pairing messages
    uint8_t extHeader[] = {0x98, 0x00};
    int payloadIndex = x3d_prepare_message_header(buffer, &msgNo, X3D_MSG_TYPE_PAIRING, 0, 0x85, extHeader, sizeof(extHeader), msgId);
    x3d_set_message_retrans(buffer, payloadIndex, replyCnt, transferSlotMask);

    // x3d_set_pairing_data(buffer, payloadIndex, 0, 0, X3D_PAIR_STATE_OPEN);
    x3d_set_pairing_data(buffer, payloadIndex, 0, 0x1234, X3D_PAIR_STATE_PINNED);
    // */

    /* Beacon message
    uint8_t extHeader[] = {0x98, 0x00};
    int payloadIndex = x3d_prepare_message_header(buffer, &msgNo, X3D_MSG_TYPE_BEACON, 0, 0x05, extHeader, sizeof(extHeader), msgId);
    x3d_set_message_retrans(buffer, payloadIndex, replyCnt, targetSlotMask);
    x3d_set_beacon_data(buffer, payloadIndex, 0);
    // */

    /* Sensor message
    uint8_t extHeader[] = { 0x82, 0x00, 0x03, 0x08, 0x12, 0x04, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x21 };
    int payloadIndex = x3d_prepare_message_header(buffer, &msgNo, X3D_MSG_TYPE_SENSOR, X3D_HEADER_FLAG_NO_RESPONSE, 0x02, extHeader, sizeof(extHeader), 0);
    x3d_set_crc(buffer);
    // */

    //* Standard message
    transferSlotMask = 0x000F;
    targetSlotMask = 0x000f;
    uint8_t extHeader[] = {0x98, 0x00};
    uint16_t data[] = {0xaaaa, 0xbbbb, 0xcccc, 0xdddd};
    int payloadIndex = x3d_prepare_message_header(buffer, &msgNo, X3D_MSG_TYPE_STANDARD, 0, 0x05, extHeader, sizeof(extHeader), msgId);
    x3d_set_message_retrans(buffer, payloadIndex, replyCnt, transferSlotMask);
    //x3d_set_register_read(buffer, payloadIndex, targetSlotMask, 0x16, 0x11);
    //x3d_set_unpair_device(buffer, payloadIndex, targetSlotMask);
    //x3d_set_ping_device(buffer, payloadIndex, targetSlotMask);
    //x3d_set_register_write_same(buffer, payloadIndex, targetSlotMask, 0x16, 0x11, 0xaaaa);
    x3d_set_register_write(buffer, payloadIndex, targetSlotMask, 0x16, 0x11, data);
    // */

    do
    {
        x3d_set_message_retrans(buffer, payloadIndex, replyCnt, transferSlotMask);
        x3d_set_crc(buffer);
        print_buffer(buffer);
        replyCnt--;

    } while (replyCnt >= 0);


        /*

    std::string line;
    std::ifstream infile("data.txt");

    uint16_t pairId = 0x1234;
    uint8_t slot = 0;

    while (std::getline(infile, line))
    {
        if (line.length() > 128)
        {
            continue;
        }
        hexToBytes(line, buffer);
        int res = process_pairing_message(buffer, pairId, &slot, 1);
        switch (res)
        {
        case X3D_PAIR_RESULT_PIN_VALID:
            pairId = 0;
            break;
        // debug reset pair
        case X3D_PAIR_RESULT_RETRANS:
            pairId = 0x1234;
            slot = 0;
            break;

        }

        for (int i = 0; i < 64; i++)
        {
            std::cout << " " << std::hex << std::setfill('0') << std::setw(2) << (int)buffer[i];
        }
        std::cout << std::dec << std::endl;
    }*/
}