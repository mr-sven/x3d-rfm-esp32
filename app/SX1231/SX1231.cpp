
#include <SmingCore.h>

#include "SX1231.h"

namespace SX1231
{
    const double_t F_SCALE = 1000000.0;
    const double_t FOSC = 32000000.0 * F_SCALE;
    const double_t FSTEP = FOSC / 524288.0; // FOSC/2^19

    SPISettings spi_settings(8000000, MSBFIRST, SPI_MODE0);

    int dioPintToIndex(DioPin dioPin)
    {
        switch (dioPin)
        {
            case DioPin::Dio0: return 0;
            case DioPin::Dio1: return 1;
            case DioPin::Dio2: return 2;
            case DioPin::Dio3: return 3;
            case DioPin::Dio4: return 4;
            case DioPin::Dio5: return 5;
        }
        return 0;
    }

    bool isDioModeMatch(Mode mode, DioMode dioMode)
    {
        switch (dioMode)
        {
            case DioMode::None: break;
            case DioMode::Rx: if (mode == Mode::Receiver) { return true; } break;
            case DioMode::Tx: if (mode == Mode::Transmitter) { return true; } break;
            case DioMode::Both: if (mode == Mode::Transmitter || mode == Mode::Receiver) { return true; } break;
        }
        return false;
    }

    Interface::Interface(uint8_t csPin, uint8_t irqPin, SPIClass *spi)
    {
        _irqPin = irqPin;
        _csPin = csPin;
        _spi = spi;
        _haveData = false;
        rfIrq = nullptr;
        _mode = Mode::Standby;
        _sequencerOff = false;

        pinMode(_csPin, OUTPUT);
        digitalWrite(_csPin, HIGH);
        pinMode(_irqPin, INPUT_PULLDOWN);
    }

    Interface::~Interface()
    {
    }

    void Interface::interruptRoutine(void)
    {
        _haveData = true;
        if (rfIrq != nullptr)
        {
            System.queueCallback(rfIrq);
        }
    }

    void Interface::select(void)
    {
        _spi->beginTransaction(spi_settings);
        digitalWrite(_csPin, LOW);
    }

    void Interface::unselect(void)
    {
        digitalWrite(_csPin, HIGH);
        _spi->endTransaction();
    }

    void Interface::initialize(void)
    {
        _irqNum = digitalPinToInterrupt(_irqPin);
        _spi->begin();

        uint32_t start = millis();
        uint8_t timeout = 50;
        do
        {
            writeReg(Registers::SyncValue1, (uint8_t)0xAA);
        } while (readReg(Registers::SyncValue1) != 0xaa && millis() - start < timeout);
        start = millis();
        do
        {
            writeReg(Registers::SyncValue1, (uint8_t)0x55);
        } while (readReg(Registers::SyncValue1) != 0x55 && millis() - start < timeout);

        ocp({.on = false, .trim = 15});
        mode(Mode::Standby);

        start = millis();
        while (((readReg(Registers::IrqFlags1) & IrqFlags1::ModeReady) == 0) && millis() - start < timeout); // wait for ModeReady

        if (millis() - start >= timeout)
        {
            return;
        }
        attachInterrupt(_irqPin, InterruptDelegate(&Interface::interruptRoutine, this), RISING);
    }

    uint8_t Interface::readReg(Registers reg)
    {
        select();
        _spi->transfer(static_cast<uint8_t>(reg) & SX1231_REGISTER_READ);
        uint8_t regval = _spi->transfer(0);
        unselect();
        return regval;
    }

    void Interface::writeReg(Registers reg, uint8_t value)
    {
        select();
#ifdef SX_DEBUG
        Serial.printf("%02x: %02x\n", static_cast<uint8_t>(reg), value);
#endif
        _spi->transfer(static_cast<uint8_t>(reg) | SX1231_REGISTER_WRITE);
        _spi->transfer(value);
        unselect();
    }

    void Interface::writeReg16(Registers reg, uint16_t value)
    {
        select();
#ifdef SX_DEBUG
        Serial.printf("%02x: %04x\n", static_cast<uint8_t>(reg), value);
#endif
        _spi->transfer(static_cast<uint8_t>(reg) | SX1231_REGISTER_WRITE);
        _spi->transfer16(value);
        unselect();
    }

    void Interface::writeReg32(Registers reg, uint32_t value, uint8_t bits)
    {
        select();
#ifdef SX_DEBUG
        Serial.printf("%02x: %08x\n", static_cast<uint8_t>(reg), value);
#endif
        _spi->transfer(static_cast<uint8_t>(reg) | SX1231_REGISTER_WRITE);
        _spi->transfer32(value, bits);
        unselect();
    }

    void Interface::writeRegBuf(Registers reg, uint8_t * value, size_t length)
    {
        select();
#ifdef SX_DEBUG
        Serial.printf("%02x: ", static_cast<uint8_t>(reg));
        for (int i = 0; i < length; i++)
        {
            Serial.printf("%02x", value[i]);
        }
        Serial.println();
#endif
        _spi->transfer(static_cast<uint8_t>(reg) | SX1231_REGISTER_WRITE);
        _spi->transfer(value, length);
        unselect();
    }

    void Interface::receiveBegin()
    {
        mode(Mode::Standby);
        _haveData = false;
        mode(Mode::Receiver);
    }

    uint8_t Interface::checkReceived(uint8_t *buffer)
    {
        if (!_haveData)
        {
            return 0;
        }
        _haveData = false;

        if (!(readReg(Registers::IrqFlags2) & static_cast<uint8_t>(IrqFlags2::PayloadReady)))
        {
            return 0;
        }

        mode(Mode::Standby);
        select();
        // payload length is returned by fifo select
        uint8_t playLoadLen = _spi->transfer(static_cast<uint8_t>(Registers::Fifo) & SX1231_REGISTER_READ);
        playLoadLen = playLoadLen > 64 ? 64 : playLoadLen; // precaution
        _spi->transfer(buffer, playLoadLen);
        unselect();
        mode(Mode::Receiver);
        return playLoadLen;
    }

    void Interface::transfer(uint8_t *buffer, uint8_t size)
    {
        mode(Mode::Standby);
        while (readReg(Registers::IrqFlags1) & static_cast<uint8_t>(IrqFlags1::ModeReady) == 0x00); // wait for ModeReady
        select();
        _spi->transfer(static_cast<uint8_t>(Registers::Fifo) | SX1231_REGISTER_WRITE);
        for (uint8_t i = 0; i < size; i++)
        {
            _spi->transfer(buffer[i]);
        }
        unselect();
        mode(Mode::Transmitter);
        while (readReg(Registers::IrqFlags2) & static_cast<uint8_t>(IrqFlags2::PacketSent) == 0x00); // wait for ModeReady
        mode(Mode::Standby);
    }

    int16_t Interface::readRSSI(void)
    {
        int16_t rssi = 0;
        rssi = -readReg(Registers::RssiValue);
        rssi >>= 1;
        return rssi;
    }

    void Interface::mode(Mode mode)
    {
        if (_mode == mode)
        {
            return;
        }
        _mode = mode;

        uint8_t data = static_cast<uint8_t>(_mode) | static_cast<uint8_t>(_sequencerOff) << 7;
        writeReg(Registers::OpMode, data);

        updateDio();
    }

    void Interface::modulation(Modulation modulation)
    {
        writeReg(Registers::DataModul, modulation.value());
    }

    void Interface::bitRate(uint32_t bitRate)
    {
        uint16_t data = (uint16_t)round(FOSC / (F_SCALE * (double)bitRate));
        writeReg16(Registers::BitrateMsb, data);
    }

    void Interface::fdev(uint32_t fdev)
    {
        uint16_t data = (uint16_t)round((F_SCALE * (double)fdev) / FSTEP);
        writeReg16(Registers::FdevMsb, data);
    }

    void Interface::frequency(uint32_t frequency)
    {
        uint32_t data = (uint32_t)round((F_SCALE * (double)frequency) / FSTEP);
        //data <<= 8;
        writeReg32(Registers::FrfMsb, data, 24);
    }

    void Interface::rxBw(RxBw rxBw)
    {
        writeReg(Registers::RxBw, rxBw.value());
    }

    void Interface::rxAfcBw(RxBw rxBw)
    {
        writeReg(Registers::AfcBw, rxBw.value());
    }

    void Interface::afcFei(bool feiStart, bool autoclearOn, bool autoOn, bool clear, bool start)
    {
        uint8_t data = static_cast<uint8_t>(feiStart) << 5 | static_cast<uint8_t>(autoclearOn) << 3 | static_cast<uint8_t>(autoOn) << 2 | static_cast<uint8_t>(clear) << 2 | static_cast<uint8_t>(start);
        writeReg(Registers::AfcFei, data);
    }

    void Interface::dioMapping(DioMapping dioMapping)
    {
        _dio[dioPintToIndex(dioMapping.pin)] = dioMapping;
        updateDio();
    }

    void Interface::clearDio(DioPin dioPin)
    {
        _dio[dioPintToIndex(dioPin)].dioMode = DioMode::None;
        updateDio();
    }

    void Interface::updateDio(void)
    {
        uint16_t data = static_cast<uint8_t>(ClkOut::OFF);
        for (int i = 0; i < SX1231_DIO_MAPPING_COUNT; i++)
        {
            if (isDioModeMatch(_mode, _dio[i].dioMode))
            {
                data |= static_cast<uint16_t>(_dio[i].dioType) << static_cast<uint16_t>(_dio[i].pin);
            }
        }
        writeReg16(Registers::DioMapping1, data);
    }

    void Interface::ocp(Ocp ocp)
    {
        writeReg(Registers::Ocp, ocp.value());
    }

    void Interface::sequencer(bool off)
    {
        if (_sequencerOff == off)
        {
            return;
        }
        _sequencerOff = off;
    }

    void Interface::rssiThreshold(uint8_t rssiThreshold)
    {
        writeReg(Registers::RssiThresh, rssiThreshold);
    }

    void Interface::preamble(uint16_t length)
    {
        writeReg16(Registers::PreambleMsb, length);
    }

    void Interface::sync(SyncConfig syncConfig, uint8_t *sync)
    {
        writeReg(Registers::SyncConfig, syncConfig.value());
        writeRegBuf(Registers::SyncValue1, sync, syncConfig.syncSize);
    }

    void Interface::packet(PacketConfig packetConfig)
    {
        writeReg(Registers::PacketConfig1, packetConfig.value1());
        writeReg(Registers::PayloadLength, packetConfig.payloadLength);
        writeReg(Registers::PacketConfig2, packetConfig.value2());
    }

    void Interface::fifoThreshold(bool fifoNotEmpty, uint8_t threshold)
    {
        threshold &= 0x7f;
        threshold |= static_cast<uint8_t>(fifoNotEmpty) << 7;
        writeReg(Registers::FifoThresh, threshold);
    }

    void Interface::sensitivityBoost(SensitivityBoost sensitivityBoost)
    {
        writeReg(Registers::TestLna, static_cast<uint8_t>(sensitivityBoost));
    }

    void Interface::continuousDagc(ContinuousDagc continuousDagc)
    {
        writeReg(Registers::TestDagc, static_cast<uint8_t>(continuousDagc));
    }

    void Interface::paLevel(bool pa0On, bool pa1On, bool pa2On, uint8_t outputPower)
    {
        outputPower &= 0x1F;
        outputPower |= static_cast<uint8_t>(pa0On) << 7;
        outputPower |= static_cast<uint8_t>(pa1On) << 6;
        outputPower |= static_cast<uint8_t>(pa2On) << 5;
        writeReg(Registers::PaLevel, outputPower);
    }

    void Interface::lna(LnaConfig lna)
    {
        writeReg(Registers::Lna, lna.value());
    }
}