#include <SmingCore.h>

#include <SPI.h>
#include <ArduinoJson.h>
#include <esp_rom_crc.h>

#include "RFM69registers.h"
#include "RFM69.h"

#define LED_PIN 2 // GPIO2

#define RFM69_CS 5
#define RFM69_IRQ 4

#define X3D_STD_MSG_TYPE       1
#define X3D_BEACON_MSG_TYPE    3

const uint8_t rfm_config[][2] = {
    {REG_OPMODE, 0x81},
    {REG_DATAMODUL, 0x00},
    // 40000 baud
    {REG_BITRATEMSB, 0x03},
    {REG_BITRATELSB, 0x20},
    // FSK Dev 80 kHz
    {REG_FDEVMSB, 0x05},
    {REG_FDEVLSB, 0x1f},
    // Carrier 868.95 MHz
    {REG_FRFMSB, 0xd9},
    {REG_FRFMID, 0x3c},
    {REG_FRFLSB, 0xcd},
    // other stuff
    //{REG_LNA, 0x88},
    {REG_RXBW, 0x42},
    {REG_AFCBW, 0x93},
    {REG_AFCFEI, 0x1C},
    {REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01},
    {REG_DIOMAPPING2, 0x07},
    {REG_RSSITHRESH, (114*2)},
    // preamble size 4 Byte
    {REG_PREAMBLELSB, 0x04},
    // sync word
    {REG_SYNCVALUE1, 0x81},
    {REG_SYNCVALUE2, 0x69},
    {REG_SYNCVALUE3, 0x96},
    {REG_SYNCVALUE4, 0x7e},
    // variable length, no CRC, Whitening
    {REG_PACKETCONFIG1, 0xc0},
    // max payload 64
    {REG_PAYLOADLENGTH, 0x40},
    {REG_PACKETCONFIG2, (5 << 4) | RF_PACKET2_RXRESTART | RF_PACKET2_AUTORXRESTART_ON},
    // other stuff
    {REG_FIFOTHRESH, 0x8f},
    {REG_TESTLNA, 0x2d},
    {REG_TESTDAGC, 0x30},
};
const char hexLookup[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

Timer procTimer;
bool state = true;

RFM69 * rfm;

uint16_t deviceId = 1234;
uint8_t msgNo = 1;

Timer sendMsgTimer;

struct messageHeaderSimple {
    uint16_t deviceId;
    uint32_t unknown;
    uint8_t addFields;
    uint32_t messageId;
    uint8_t retryCount;
} __attribute__((packed));

struct messageHeaderTemp {
    uint16_t deviceId;
    uint32_t unknown;
    uint8_t addFields;
    uint8_t tempSign;
    uint16_t temperature;
    uint32_t messageId;
    uint8_t retryCount;
} __attribute__((packed));

struct registerData {
    uint16_t reg;
    uint16_t status;
    uint16_t value;
} __attribute__((packed));

struct beaconMessage {
    uint8_t actor;
    uint8_t unknown1;
    uint8_t response;
    uint16_t unknown2;
    uint16_t unknown3;
    uint16_t unknown4;
} __attribute__((packed));

void blink()
{
    digitalWrite(LED_PIN, state);
    state = !state;
}

uint32_t recvMessageId = 0;
uint8_t recvMsgNo = 0;

void addRaw(uint8_t * buffer, uint8_t bufLen, DynamicJsonDocument &doc)
{
    String hexStr;
    for (uint8_t x = 0; x < bufLen; x++) 
    { 
        hexStr += hexLookup[(buffer[x] >> 4) & 0xF]; 
        hexStr += hexLookup[buffer[x] & 0xF]; 
    } 
    doc["raw"] = hexStr;
}

void parseStdMessage(uint8_t * buffer, uint8_t bufLen, DynamicJsonDocument &doc)
{
    doc["targetActor"] = buffer[0];
    doc["response"] = buffer[2] & 0x01 == 0x01;
    uint8_t subMessageType = buffer[6];
    doc["subMessageType"] = subMessageType;
    if (subMessageType == 1 || subMessageType == 9)
    {
        struct registerData reg = {0};
        memcpy(&reg, &buffer[7], sizeof(reg));
        doc["register"] = String(reg.reg, HEX);
        doc["status"] = reg.status;
        doc["value"] = String(reg.value, HEX);
    }
    else if (subMessageType != 8)
    {
        addRaw(buffer, bufLen, doc);
    }
}

void parseBeaconMessage(uint8_t * buffer, uint8_t bufLen, DynamicJsonDocument &doc)
{
    struct beaconMessage beacon = {0};
    memcpy(&beacon, buffer, sizeof(beacon));
    doc["targetActor"] = beacon.actor;
    doc["response"] = beacon.response & 0x01 == 0x01;
}

bool parsePacket(uint8_t * buffer, uint8_t bufLen, DynamicJsonDocument &doc)
{
    if (bufLen < 4)
    {
        return false;
    }
    
    if (buffer[0] != 0xff)
    {
        return false;
    }

    uint8_t msgNo = buffer[1];
    doc["msgNo"] = msgNo;
    uint8_t msgType = buffer[2];
    uint8_t headerLen = buffer[3];

    doc["msgType"] = msgType;

    struct messageHeaderSimple simpleHeader = {0};
    struct messageHeaderTemp tempHeader = {0};

    if (headerLen == sizeof(simpleHeader))
    {
        memcpy(&simpleHeader, &buffer[4], sizeof(simpleHeader));
        doc["deviceId"] = simpleHeader.deviceId;
        doc["msgId"] = simpleHeader.messageId;
        doc["retry"] = simpleHeader.retryCount;
        if (simpleHeader.deviceId == deviceId)
        {
            recvMessageId = simpleHeader.messageId;
            recvMsgNo = msgNo;
        }
    }
    else if (headerLen == sizeof(tempHeader))
    {
        memcpy(&tempHeader, &buffer[4], sizeof(tempHeader));
        doc["deviceId"] = tempHeader.deviceId;
        doc["msgId"] = tempHeader.messageId;
        doc["tempSign"] = tempHeader.tempSign;
        doc["temperature"] = tempHeader.temperature / 100.0;
        if (tempHeader.deviceId == deviceId)
        {
            recvMessageId = tempHeader.messageId;
            recvMsgNo = msgNo;
        }
    }
    else
    {
        return false;
    }

    switch(msgType)
    {
        case X3D_STD_MSG_TYPE:
            parseStdMessage(&buffer[headerLen + 4], bufLen - headerLen - 4, doc);
            break;
        case X3D_BEACON_MSG_TYPE:
            parseBeaconMessage(&buffer[headerLen + 4], bufLen - headerLen - 4, doc);
            break;
        case 0x02:
        default:
            addRaw(&buffer[headerLen + 4], bufLen - headerLen - 4, doc);
            break;
    }

    return true;
}

void rfmIrq(void)
{
    uint8_t buffer[64];
    uint8_t bufLen = rfm->checkReceived(buffer);
    if (bufLen < 3) // CRC + length error by deltadore
    {
        return;
    }

    // add len to crc
    uint16_t crc = ~esp_rom_crc16_be(~0x0000, &bufLen, 1);

    // no CRC and no garbage
    bufLen -= 3;

    uint16_t rxcrc = buffer[bufLen] << 8 | buffer[bufLen + 1];

    crc = ~esp_rom_crc16_be(~crc, buffer, bufLen);
    if (rxcrc != crc)
    {
        return;
    }

    DynamicJsonDocument doc(1024);

    if (!parsePacket(buffer, bufLen, doc))
    {
        addRaw(buffer, bufLen, doc);
    }

    serializeJson(doc, Serial);
    Serial.println();
}

void set_crc(uint8_t * buffer, size_t buffer_len)
{
    uint16_t crc = ~esp_rom_crc16_be(~0x0000, buffer, buffer_len - 2);
    buffer[buffer_len - 2] = (crc >> 8) & 0xff;
    buffer[buffer_len - 1] = crc & 0xff;
}

struct messageHeaderSimple sendHeader = {0};
uint8_t sendBuffer[64] = {0};
uint8_t sendBufferLen = 0;

void sendMsgCallback()
{
    if (recvMessageId == sendHeader.messageId && recvMsgNo == sendBuffer[2])
    {
        return;
    }
    sendHeader.retryCount--;
    memcpy(&sendBuffer[5], &sendHeader, sizeof(sendHeader));
    set_crc(sendBuffer, sendBufferLen - 1); // -1 to fix X3D length bug

    rfm->transfer(sendBuffer, sendBufferLen);
    rfm->receiveBegin();

    if (sendHeader.retryCount > 0)
    {
        sendMsgTimer.startOnce();
    }
}

void send_message(uint8_t msg_type, uint8_t retry, void * data, size_t data_length)
{
    recvMessageId = 0;
    recvMsgNo = 0;

    sendHeader.deviceId = deviceId;
    sendHeader.unknown = 0x98050040;
    sendHeader.addFields = 0x00;
    sendHeader.messageId = esp_random();    
    sendHeader.retryCount = retry + 1;

    sendBufferLen = 5 + sizeof(sendHeader) + data_length + 3;
    
    sendBuffer[0] = sendBufferLen - 1;
    sendBuffer[1] = 0xff;
    sendBuffer[2] = msgNo++;
    sendBuffer[3] = msg_type;
    sendBuffer[4] = sizeof(sendHeader);

    memcpy(&sendBuffer[5 + sizeof(sendHeader)], data, data_length);
    sendMsgTimer.startOnce();
}

void send_beacon_msg(uint8_t actor)
{
    struct beaconMessage beacon = {0};
    beacon.actor = actor;
    beacon.unknown1 = 0x00;
    beacon.response = 0;
    beacon.unknown2 = 0xff00;
    beacon.unknown3 = 0x0000;
    beacon.unknown4 = 0xffe0;

    send_message(X3D_BEACON_MSG_TYPE, 4, &beacon, sizeof(beacon));
}

void init()
{
    rfm = new RFM69(RFM69_CS, RFM69_IRQ, new SPIClass(SpiBus::VSPI));

    sendMsgTimer.setIntervalMs(100);
    sendMsgTimer.setCallback(sendMsgCallback);

    Serial.setTxBufferSize(2048);
    Serial.setTxWait(false);
    Serial.begin(SERIAL_BAUD_RATE, SERIAL_8N1, SERIAL_FULL, SERIAL_PIN_DEFAULT, SERIAL_PIN_DEFAULT);

    rfm->rfIrq = rfmIrq;
    rfm->initialize(rfm_config, sizeof(rfm_config) / 2);

    Serial.print(_F("\r\n== INIT FINISH ==\r\n"));
    pinMode(LED_PIN, OUTPUT);
    procTimer.initializeMs(1000, blink).start();
    
    rfm->receiveBegin();

    //send_beacon_msg(1);
}
