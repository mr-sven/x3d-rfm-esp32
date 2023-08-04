#include <SmingCore.h>

#include <soc/spi_pins.h>
#include <esp_rom_crc.h>

#include "config.h"

#include "SX1231/SX1231.h"

SX1231::Interface * sx;
SPIClass RFM_Spi;

#if BLINK_ALIVE_LED
Timer procTimer;
bool state = true;
void blink()
{
    digitalWrite(LED_PIN, state);
    state = !state;
}
#endif

String toHexString(uint8_t * buffer, uint8_t bufLen)
{
    const char hexLookup[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    String hexStr;
    for (uint8_t x = 0; x < bufLen; x++)
    {
        hexStr += hexLookup[(buffer[x] >> 4) & 0xF];
        hexStr += hexLookup[buffer[x] & 0xF];
    }
    return hexStr;
}

void rfmIrq(void)
{
    uint8_t buffer[64];
    uint8_t bufLen = sx->checkReceived(buffer);
    if (bufLen < 3) // CRC + length error by deltadore
    {
        return;
    }

    uint16_t crc = ~esp_rom_crc16_be(~0x0000, buffer, bufLen - 2);
    uint16_t rxcrc = buffer[bufLen - 2] << 8 | buffer[bufLen - 1];

    if (rxcrc != crc)
    {
        return;
    }

#if CONSOLE_RAW_OUTPUT
    Serial << toHexString(buffer, bufLen) << endl;
#endif
}

#if USE_WIFI
// Will be called when WiFi station was connected to AP
void connectOk(IpAddress ip, IpAddress mask, IpAddress gateway)
{
    Serial << _F("Wifi connected with ") << ip << endl;
}

// Will be called when WiFi station was disconnected
void connectFail(const String& ssid, MacAddress bssid, WifiDisconnectReason reason)
{
    Serial << _F("Wifi disconnect from: ") << ssid << _F(" reason: ") << WifiEvents.getDisconnectReasonDesc(reason) << endl;
    System.queueCallback([](void* param) {
        delay(10000);
        WifiStation.connect();
    });
}
#endif


void init()
{
    Serial.setTxBufferSize(2048);
    Serial.setTxWait(false);
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.systemDebugOutput(true);

    Serial << _F("== INIT START ==") << endl;

    RFM_Spi.setup(SpiBus::VSPI, RFM_SPI_PINS);
    sx = new SX1231::Interface(RFM69_CS, RFM69_IRQ, &RFM_Spi);

    sx->rfIrq = rfmIrq;
    sx->initialize();
    sx->modulation({.dataMode = SX1231::DataMode::Packet, .modulationType = SX1231::ModulationType::Fsk, .shaping = SX1231::ModulationShaping::Shaping00});
    sx->bitRate(40000);
    sx->fdev(80000);
    sx->frequency(868950000U);
    sx->rxBw({.dccCutoff = SX1231::DccCutoff::Percent4, .rxBw = static_cast<uint8_t>(SX1231::RxBwFsk::Khz125dot0)});
    sx->rxAfcBw({.dccCutoff = SX1231::DccCutoff::Percent1, .rxBw = static_cast<uint8_t>(SX1231::RxBwFsk::Khz41dot7)});
    sx->afcFei(false, true, true, true, false);
    sx->dioMapping({.pin = SX1231::DioPin::Dio0, .dioType = SX1231::DioType::Dio01, .dioMode = SX1231::DioMode::Both});
    sx->rssiThreshold(114*2);
    sx->preamble(4);

    uint8_t sync[] = {0x81, 0x69, 0x96, 0x7e};
    sx->sync({.syncSize = sizeof(sync)}, sync);
    sx->packet({
        .format = SX1231::PacketFormat::Variable,
        .dcFree = SX1231::PacketDc::Whitening,
        .payloadLength = 64,
        .crcOn = false,
        .crcAutoClearOff = false,
        .filtering = SX1231::PacketFiltering::None,
        .interPacketRxDelay = SX1231::InterPacketRxDelay::Delay32Bits,
        .autoRxRestart = true});
    sx->fifoThreshold(true, 15);
    sx->sensitivityBoost(SX1231::SensitivityBoost::HighSensitivity);
    sx->continuousDagc(SX1231::ContinuousDagc::ImprovedMarginAfcLowBetaOn0);
    sx->mode(SX1231::Mode::Standby);
    sx->paLevel(false, true, true, 23);
    sx->receiveBegin();

#if USE_WIFI
    WifiAccessPoint.enable(false);
    WifiStation.enable(true);
    WifiStation.config(_F(WIFI_SSID), _F(WIFI_PWD));
    WifiEvents.onStationGotIP(connectOk);
    WifiEvents.onStationDisconnect(connectFail);
#endif

    Serial << _F("== INIT FINISH ==") << endl;

#if BLINK_ALIVE_LED
    pinMode(LED_PIN, OUTPUT);
    procTimer.initializeMs(1000, blink).start();
#endif
}
