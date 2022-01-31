#ifndef RFM69_H
#define RFM69_H

#include <SPI.h>

class RFM69
{
private:
    uint8_t _csPin;
    uint8_t _irqPin;
    uint8_t _irqNum;
    volatile bool _haveData;

    SPIClass *_spi;
    void select(void);
    void unselect(void);
    void writeReg(uint8_t addr, uint8_t val);
    uint8_t readReg(uint8_t addr);
    void interruptRoutine(void);

public:
    void (*rfIrq)(void);

    RFM69(uint8_t csPin, uint8_t irqPin, SPIClass *spi = nullptr);
    ~RFM69();

    void initialize(const uint8_t config[][2], size_t count);
    void setOpMode(uint8_t mode);

    void receiveBegin(void);
    uint8_t checkReceived(uint8_t *buffer);
    void transfer(uint8_t *buffer, uint8_t size);
    int16_t readRSSI(void);
};
#endif