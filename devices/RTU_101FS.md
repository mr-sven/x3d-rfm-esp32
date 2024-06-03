# Multifrenchise RTU 101FS

This device is mostly not direct available via Delta Dore, it is offered by diffrent sellers.

## Ids

* 7729936

## Chips

* µPD78F1503A - RENESAS 78K0R/LG3 20 MHz, 64 KB ROM, 4 KB SRAM, LQFP100
* SX1211 - Ultra Low Power (3 mA RX) RF Transceiver 862-960 MHz
* M24C08 - ST IC EEPROM 8KBIT I2C, UFDFPN8

## PCB

![TOP](img/RTU_101FS_top.jpg)
![BOTTOM](img/RTU_101FS_bottom.jpg)

## Main Controller Pinout

The TPs GND, VCC, RxD0, TxD0 and Reset are accessible via back cover, so I assume the devices are factory programmed with a Bootloader so later programming can be done via UART.

* Pin 4: INTP11 used for button press detection
* Pin 24 - 25: I2C SCL0 SDA0 connected to M24C08 EEPROM
* Pin 26: Default backlight
* Pin 27: Possible general backlight brightness controller
* Pin 28: Additional backlight color (Some devices are described with backlight color in operation mode)
* Pin 29: Additional backlight color (Some devices are described with backlight color in operation mode)
* Pin 31 - 70: are fully used for the LCD Driver. COM0 - COM3, COM4/SEG0 - COM7/SEG3 and SEG4-SEG34
* Pin 77: ANI10 Connected to Thermal Resistor
* Pin 84 - 87: P20 - P23 used for button output
  * In general all pins are at low level if the interupt INTP11 is detected or the device is in Active mode, the buttons are pulled up on a loop to detect the Pressed button.
* Pin 80 - 83: P24 - P27 used as rotary encoder inputs
* Pin 88: P130 used to control the encoder power
  * This pin is raised once per second for 4µs to check the rotary encoder position and save power.
* Pin 91: PCLBUZ0 connected to the buzzer
* Pin 92 - 100: connected to SX1211
  * INTP2 - IRQ_0
  * INTP1 - IRQ_1
  * P16 - DATA
  * SCK10 - SCK
  * SI10 - MISO
  * SO10 - MOSI
  * P12 - NSS_DATA
  * P11 - NSS_CONFIG
  * P10 - TEST8(POR)