#ifndef __SX1231_H__
#define __SX1231_H__

#include <SPI.h>
#include "SX1231_Register.h"

namespace SX1231
{
    class Interface
    {
    private:
        uint8_t _csPin;
        uint8_t _irqPin;
        uint8_t _irqNum;
        volatile bool _haveData;
        Mode _mode;
        DioMapping _dio[SX1231_DIO_MAPPING_COUNT];
        SPIClass *_spi;
        bool _sequencerOff;

        void select(void);
        void unselect(void);
        uint8_t readReg(Registers reg);
        void writeReg(Registers reg, uint8_t value);
        void writeReg16(Registers reg, uint16_t value);
        void writeReg32(Registers reg, uint32_t value, uint8_t bits = 32);
        void writeRegBuf(Registers reg, uint8_t * value, size_t length);

        void interruptRoutine(void);
        void updateDio(void);

    public:
        void (*rfIrq)(void);

        Interface(uint8_t csPin, uint8_t irqPin, SPIClass *spi = nullptr);
        ~Interface();

        void initialize(void);

        void sequencer(bool off);
        void mode(Mode mode);
        void modulation(Modulation modulation);
        void bitRate(uint32_t bitRate);
        void fdev(uint32_t fdev);
        void frequency(uint32_t frequency);
        void rxBw(RxBw rxBw);
        void rxAfcBw(RxBw rxBw);
        void afcFei(bool feiStart, bool autoclearOn, bool autoOn, bool clear, bool start);
        void dioMapping(DioMapping dioMapping);
        void clearDio(DioPin dioPin);
        void ocp(Ocp ocp);
        void rssiThreshold(uint8_t rssiThreshold);
        void preamble(uint16_t length);
        void sync(SyncConfig syncConfig, uint8_t *sync);
        void packet(PacketConfig packetConfig);
        void fifoThreshold(bool fifoNotEmpty, uint8_t threshold);
        void sensitivityBoost(SensitivityBoost sensitivityBoost);
        void continuousDagc(ContinuousDagc continuousDagc);
        void paLevel(bool pa0On, bool pa1On, bool pa2On, uint8_t outputPower);
        void lna(LnaConfig lna);

        void receiveBegin(void);
        uint8_t checkReceived(uint8_t *buffer);
        void transfer(uint8_t *buffer, uint8_t size);
        int16_t readRSSI(void);
    };
}

#endif // __SX1231_H__