# X3D RFM69 ESP Interface

Note: work in progress
- Modulation: FSK PCM
- Frequency: 868.95 MHz
- FSK Frequency Deviation: 80 kHz
- 25 us bit time
- 40000 baud
- based on Semtech SX1211
- manual CRC
- CCITT Data Whitening

Payload format:
- Preamble          {32} 0xaaaaaaaa
- Syncword          {32} 0x8169967e
- Length            {8}
- Payload           {n}
- CRC16(Poly=0x1021,Init=0x0000)

The RFM69HW module is based on Semtech SX1231H. The nearest target frequency is 868.950012 MHz, the internal AFC is able to take care on this gap.

Based on Sming 4.5 Framework: [https://github.com/SmingHub/Sming](https://github.com/SmingHub/Sming)

## Wireing

used:
- DOIT ESP32 DEVKIT V1
- RFM69HW 868 MHz

| ESP32 | RFM69 |
|-------|-------|
| 3V3   | 3.3V  |
| GND   | GND   |
| D4    | DIO0  |
| D5    | NSS   |
| D18   | SCK   |
| D19   | MISO  |
| D23   | MOSI  |

[<img src="x3d-rfm-esp32.png" width="400"/>](x3d-rfm-esp32.png)


## SDK Config

`Component config -> ESP System Settings`

```bash
make SMING_ARCH=Esp32 sdk-menuconfig
...
make SMING_ARCH=Esp32 Sming-build all
```