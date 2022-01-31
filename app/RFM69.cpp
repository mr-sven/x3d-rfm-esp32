#include <SmingCore.h>

#include "RFM69.h"
#include "RFM69registers.h"

SPISettings spi_settings(8000000, MSBFIRST, SPI_MODE0);

RFM69::RFM69(uint8_t csPin, uint8_t irqPin, SPIClass *spi)
{
    _irqPin = irqPin;
    _csPin = csPin;
    _spi = spi;
    _haveData = false;
    rfIrq = nullptr;

    pinMode(_csPin, OUTPUT);
    digitalWrite(_csPin, HIGH);
    pinMode(_irqPin, INPUT_PULLDOWN);
}

RFM69::~RFM69()
{
}

void RFM69::interruptRoutine(void)
{
    _haveData = true;
    if (rfIrq != nullptr)
    {
        System.queueCallback(rfIrq);
    }
}

void RFM69::initialize(const uint8_t config[][2], size_t count)
{
    _irqNum = digitalPinToInterrupt(_irqPin);
    _spi->begin();

    uint32_t start = millis();
    uint8_t timeout = 50;
    do
    {
        writeReg(REG_SYNCVALUE1, 0xAA);
    } while (readReg(REG_SYNCVALUE1) != 0xaa && millis() - start < timeout);
    start = millis();
    do
    {
        writeReg(REG_SYNCVALUE1, 0x55);
    } while (readReg(REG_SYNCVALUE1) != 0x55 && millis() - start < timeout);

    for (size_t i = 0; i < count; i++)
    {
        writeReg(config[i][0], config[i][1]);
    }

    writeReg(REG_OCP, RF_OCP_OFF);

    setOpMode(RF_OPMODE_STANDBY);
    writeReg(REG_PALEVEL, RF_PALEVEL_PA1_ON | RF_PALEVEL_PA2_ON | 23);

    start = millis();
    while (((readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00) && millis() - start < timeout); // wait for ModeReady
    if (millis() - start >= timeout)
    {
        return;
    }
    attachInterrupt(_irqPin, InterruptDelegate(&RFM69::interruptRoutine, this), RISING);
}

uint8_t RFM69::readReg(uint8_t addr)
{
    select();
    _spi->transfer(addr & 0x7F);
    uint8_t regval = _spi->transfer(0);
    unselect();
    return regval;
}

void RFM69::writeReg(uint8_t addr, uint8_t value)
{
    select();
    _spi->transfer(addr | 0x80);
    _spi->transfer(value);
    unselect();
}

void RFM69::select(void)
{
    _spi->beginTransaction(spi_settings);
    digitalWrite(_csPin, LOW);
}

void RFM69::unselect(void)
{
    digitalWrite(_csPin, HIGH);
    _spi->endTransaction();
}

void RFM69::receiveBegin()
{
    setOpMode(RF_OPMODE_STANDBY);
    _haveData = false;
    setOpMode(RF_OPMODE_RECEIVER);
}

uint8_t RFM69::checkReceived(uint8_t *buffer)
{
    if (!_haveData)
    {
        return 0;
    }
    _haveData = false;

    if (!(readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PAYLOADREADY))
    {
        return 0;
    }

    setOpMode(RF_OPMODE_STANDBY);
    select();
    _spi->transfer(REG_FIFO & 0x7F);
    uint8_t playLoadLen = _spi->transfer(0);
    playLoadLen = playLoadLen > 64 ? 64 : playLoadLen; // precaution
    for (uint8_t i = 0; i < playLoadLen; i++)
    {
        buffer[i] = _spi->transfer(0);
    }
    unselect();
    setOpMode(RF_OPMODE_RECEIVER);
    return playLoadLen;
}

void RFM69::transfer(uint8_t *buffer, uint8_t size)
{
    setOpMode(RF_OPMODE_STANDBY);
    while ((readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00); // wait for ModeReady
    select();
    _spi->transfer(REG_FIFO | 0x80);
    for (uint8_t i = 0; i < size; i++)
    {
        _spi->transfer(buffer[i]);
    }
    unselect();
    setOpMode(RF_OPMODE_TRANSMITTER);
    while ((readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PACKETSENT) == 0x00); // wait for ModeReady
    setOpMode(RF_OPMODE_STANDBY);
}

void RFM69::setOpMode(uint8_t mode)
{
    writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | mode);
}

int16_t RFM69::readRSSI(void)
{
    int16_t rssi = 0;
    rssi = -readReg(REG_RSSIVALUE);
    rssi >>= 1;
    return rssi;
}
