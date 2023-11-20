#ifndef __SX1231_REGISTER_H__
#define __SX1231_REGISTER_H__

namespace SX1231
{
    #define SX1231_DIO_MAPPING_COUNT    6
    #define SX1231_REGISTER_WRITE       (1 << 7)
    #define SX1231_REGISTER_READ        (SX1231_REGISTER_WRITE - 1)

    // **********************************************************************************
    // RFM69/SX1231 Internal registers addresses
    //**************************************************
    enum class Registers : uint8_t {
        Fifo = 0x00,
        OpMode = 0x01,
        DataModul = 0x02,
        BitrateMsb = 0x03,
        BitrateLsb = 0x04,
        FdevMsb = 0x05,
        FdevLsb = 0x06,
        FrfMsb = 0x07,
        FrfMid = 0x08,
        FrfLsb = 0x09,
        Osc1 = 0x0A,
        AfcCtrl = 0x0B,
        LowBat = 0x0C,
        Listen1 = 0x0D,
        Listen2 = 0x0E,
        Listen3 = 0x0F,
        Version = 0x10,
        PaLevel = 0x11,
        PaRamp = 0x12,
        Ocp = 0x13,
        AgcRef = 0x14,      // not present on RFM69/SX1231
        AgcThresh1 = 0x15,  // not present on RFM69/SX1231
        AgcThresh2 = 0x16,  // not present on RFM69/SX1231
        AgcThresh3 = 0x17,  // not present on RFM69/SX1231
        Lna = 0x18,
        RxBw = 0x19,
        AfcBw = 0x1A,
        OokPeak = 0x1B,
        OokAvg = 0x1C,
        OokFix = 0x1D,
        AfcFei = 0x1E,
        AfcMsb = 0x1F,
        AfcLsb = 0x20,
        FeiMsb = 0x21,
        FeiLsb = 0x22,
        RssiConfig = 0x23,
        RssiValue = 0x24,
        DioMapping1 = 0x25,
        DioMapping2 = 0x26,
        IrqFlags1 = 0x27,
        IrqFlags2 = 0x28,
        RssiThresh = 0x29,
        RxTimeout1 = 0x2A,
        RxTimeout2 = 0x2B,
        PreambleMsb = 0x2C,
        PreambleLsb = 0x2D,
        SyncConfig = 0x2E,
        SyncValue1 = 0x2F,
        SyncValue2 = 0x30,
        SyncValue3 = 0x31,
        SyncValue4 = 0x32,
        SyncValue5 = 0x33,
        SyncValue6 = 0x34,
        SyncValue7 = 0x35,
        SyncValue8 = 0x36,
        PacketConfig1 = 0x37,
        PayloadLength = 0x38,
        NodeAddrs = 0x39,
        BroadcastAddrs = 0x3A,
        AutoModes = 0x3B,
        FifoThresh = 0x3C,
        PacketConfig2 = 0x3D,
        AesKey1 = 0x3E,
        AesKey2 = 0x3F,
        AesKey3 = 0x40,
        AesKey4 = 0x41,
        AesKey5 = 0x42,
        AesKey6 = 0x43,
        AesKey7 = 0x44,
        AesKey8 = 0x45,
        AesKey9 = 0x46,
        AesKey10 = 0x47,
        AesKey11 = 0x48,
        AesKey12 = 0x49,
        AesKey13 = 0x4A,
        AesKey14 = 0x4B,
        AesKey15 = 0x4C,
        AesKey16 = 0x4D,
        Temp1 = 0x4E,
        Temp2 = 0x4F,
        TestLna = 0x58,
        TestPa1 = 0x5A,
        TestPa2 = 0x5C,
        TestDagc = 0x6F,
    };

    enum class Mode : uint8_t {
        Sleep = (0 << 2),
        Standby = (1 << 2),
        FrequencySynthesizer = (2 << 2),
        Transmitter = (3 << 2),
        Receiver = (4 << 2),
        ListenMode = (1 << 6),
    };

    enum class DataMode : uint8_t {
        Packet = 0x00,
        Continuous = 0x40,
        ContinuousBitSync = 0x60,
    };

    enum class ModulationType : uint8_t {
        Fsk = 0x00,
        Ook = 0x08,
    };

    enum class ModulationShaping : uint8_t {
        Shaping00 = 0x00,
        Shaping01 = 0x01,
        Shaping10 = 0x02,
        Shaping11 = 0x03,
    };

    struct Modulation {
        DataMode dataMode;
        ModulationType modulationType;
        ModulationShaping shaping;
        uint8_t value()
        {
            return static_cast<uint8_t>(dataMode) | static_cast<uint8_t>(modulationType) | static_cast<uint8_t>(shaping);
        }
    };

    enum class DccCutoff : uint8_t {
        Percent16 = 0x00,
        Percent8 = 0x20,
        Percent4 = 0x40,
        Percent2 = 0x60,
        Percent1 = 0x80,
        Percent0dot5 = 0xA0,
        Percent0dot25 = 0xC0,
        Percent0dot125 = 0xE0,
    };

    enum class RxBwFsk : uint8_t {
        Khz2dot6 = 0b10 << 3 | 7,
        Khz3dot1 = 0b01 << 3 | 7,
        Khz3dot9 = 7,
        Khz5dot2 = 0b10 << 3 | 6,
        Khz6dot3 = 0b01 << 3 | 6,
        Khz7dot8 = 6,
        Khz10dot4 = 0b10 << 3 | 5,
        Khz12dot5 = 0b01 << 3 | 5,
        Khz15dot6 = 5,
        Khz20dot8 = 0b10 << 3 | 4,
        Khz25dot0 = 0b01 << 3 | 4,
        Khz31dot3 = 4,
        Khz41dot7 = 0b10 << 3 | 3,
        Khz50dot0 = 0b01 << 3 | 3,
        Khz62dot5 = 3,
        Khz83dot3 = 0b10 << 3 | 2,
        Khz100dot0 = 0b01 << 3 | 2,
        Khz125dot0 = 2,
        Khz166dot7 = 0b10 << 3 | 1,
        Khz200dot0 = 0b01 << 3 | 1,
        Khz250dot0 = 1,
        Khz333dot3 = 0b10 << 3,
        Khz400dot0 = 0b01 << 3,
        Khz500dot0 = 0,
    };

    enum class RxBwOok : uint8_t {
        Khz1dot3 = 0b10 << 3 | 7,
        Khz1dot6 = 0b01 << 3 | 7,
        Khz2dot0 = 7,
        Khz2dot6 = 0b10 << 3 | 6,
        Khz3dot1 = 0b01 << 3 | 6,
        Khz3dot9 = 6,
        Khz5dot2 = 0b10 << 3 | 5,
        Khz6dot3 = 0b01 << 3 | 5,
        Khz7dot8 = 5,
        Khz10dot4 = 0b10 << 3 | 4,
        Khz12dot5 = 0b01 << 3 | 4,
        Khz15dot6 = 4,
        Khz20dot8 = 0b10 << 3 | 3,
        Khz25dot0 = 0b01 << 3 | 3,
        Khz31dot3 = 3,
        Khz41dot7 = 0b10 << 3 | 2,
        Khz50dot0 = 0b01 << 3 | 2,
        Khz62dot5 = 2,
        Khz83dot3 = 0b10 << 3 | 1,
        Khz100dot0 = 0b01 << 3 | 1,
        Khz125dot0 = 1,
        Khz166dot7 = 0b10 << 3,
        Khz200dot0 = 0b01 << 3,
        Khz250dot0 = 0,
    };

    struct RxBw {
        DccCutoff dccCutoff;
        uint8_t rxBw;
        uint8_t value()
        {
            return static_cast<uint8_t>(dccCutoff) | rxBw;
        }
    };

    struct Ocp {
        bool on = true;
        uint8_t trim = 10;
        uint8_t value()
        {
            return (static_cast<uint8_t>(trim) & 0x0F) | static_cast<uint8_t>(on) << 4;
        }
    };

    enum class ClkOut : uint8_t {
        FXOSC = 0,
        FXOSC_2 = 1,
        FXOSC_4 = 2,
        FXOSC_8 = 3,
        FXOSC_16 = 4,
        FXOSC_32 = 5,
        RC = 6,
        OFF = 7,
    };

    enum class DioPin : uint8_t {
        Dio0 = 14,
        Dio1 = 12,
        Dio2 = 10,
        Dio3 = 8,
        Dio4 = 6,
        Dio5 = 4,
    };

    enum class DioType : uint8_t {
        Dio00 = 0b00,
        Dio01 = 0b01,
        Dio10 = 0b10,
        Dio11 = 0b11,
    };

    enum class DioMode {
        Rx,
        Tx,
        Both,
        None,
    };

    struct DioMapping {
        DioPin pin;
        DioType dioType;
        DioMode dioMode = DioMode::None;
    };

    enum class IrqFlags1 : uint8_t {
        SyncAddressMatch = 0x01,
        AutoMode = 0x02,
        Timeout = 0x04,
        Rssi = 0x08,
        PllLock = 0x10,
        TxReady = 0x20,
        RxReady = 0x40,
        ModeReady = 0x80,
    };

    inline uint8_t operator&(uint8_t a, IrqFlags1 b)
    {
        return a & static_cast<uint8_t>(b);
    }

    enum class IrqFlags2 : uint8_t {
        CrcOk = 0x02,
        PayloadReady = 0x04,
        PacketSent = 0x08,
        FifoOverrun = 0x10,
        FifoLevel = 0x20,
        FifoNotEmpty = 0x40,
        FifoFull = 0x80,
    };

    inline uint8_t operator&(uint8_t a, IrqFlags2 b)
    {
        return a & static_cast<uint8_t>(b);
    }

    struct SyncConfig {
        bool syncOn = true;
        bool fifoFillCondition = false;
        uint8_t syncSize;
        uint8_t syncTol = 0;
        uint8_t value()
        {
            return static_cast<uint8_t>(syncOn) << 7 |
                static_cast<uint8_t>(fifoFillCondition) << 6 |
                ((syncSize - 1) & 0x07) << 3 |
                ((syncTol) & 0x07);
        }
    };

    enum class InterPacketRxDelay : uint8_t {
        Delay1Bit = 0x00,
        Delay2Bits = 0x10,
        Delay4Bits = 0x20,
        Delay8Bits = 0x30,
        Delay16Bits = 0x40,
        Delay32Bits = 0x50,
        Delay64Bits = 0x60,
        Delay128Bits = 0x70,
        Delay256Bits = 0x80,
        Delay512Bits = 0x90,
        Delay1024Bits = 0xA0,
        Delay2048Bits = 0xB0,
    };

    enum class PacketFiltering : uint8_t {
        None = 0x00,
        Address = 0x02,
        Broadcast = 0x04,
    };

    enum class PacketDc : uint8_t {
        None = 0x00,
        Manchester = 0x20,
        Whitening = 0x40,
    };

    enum class PacketFormat : uint8_t {
        Variable = 0x80,
        Fixed = 0x00,
    };

    struct PacketConfig {
        PacketFormat format;
        PacketDc dcFree;
        uint8_t payloadLength = 64;
        bool crcOn = true;
        bool crcAutoClearOff = false;
        PacketFiltering filtering;
        InterPacketRxDelay interPacketRxDelay;
        bool autoRxRestart = true;
        bool aes = false;
        uint8_t value1()
        {
            return static_cast<uint8_t>(format) |
                static_cast<uint8_t>(dcFree) |
                static_cast<uint8_t>(crcOn) << 4 |
                static_cast<uint8_t>(crcAutoClearOff) << 3 |
                static_cast<uint8_t>(filtering);
        }
        uint8_t value2()
        {
            return static_cast<uint8_t>(interPacketRxDelay) |
                static_cast<uint8_t>(autoRxRestart) << 1 |
                static_cast<uint8_t>(aes);
        }
    };

    enum class SensitivityBoost : uint8_t {
        Normal = 0x1B,
        HighSensitivity = 0x2D,
    };

    enum class ContinuousDagc : uint8_t {
        Normal = 0x00,
        ImprovedMarginAfcLowBetaOn1 = 0x20,
        ImprovedMarginAfcLowBetaOn0 = 0x30,
    };

    enum class LnaImpedance : uint8_t {
        Ohm50 = 0x00,
        Ohm200 = 0x80,
    };

    enum class LnaGain : uint8_t {
        AgcLoop = 0b000,
        G1 = 0b001,
        G2 = 0b010,
        G3 = 0b011,
        G4 = 0b100,
        G5 = 0b101,
        G6 = 0b110,
    };

    struct LnaConfig {
        LnaImpedance zin;
        LnaGain gain_select;
        uint8_t value()
        {
            return static_cast<uint8_t>(zin) | static_cast<uint8_t>(gain_select);
        }
    };
}

#endif // __SX1231_REGISTER_H__