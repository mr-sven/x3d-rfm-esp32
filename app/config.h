#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <SmingCore.h>
#include <soc/spi_pins.h>

// enable to output each raw packet at console
#define CONSOLE_RAW_OUTPUT   1

// enable blink led as alive indicator
#define BLINK_ALIVE_LED      0
#if BLINK_ALIVE_LED
    #define LED_PIN 2 // GPIO2
#endif

#define RFM69_CS   5
#define RFM69_IRQ  4

constexpr struct SpiPins RFM_SPI_PINS = {
	.sck = SPI3_IOMUX_PIN_NUM_CLK,
    .miso = SPI3_IOMUX_PIN_NUM_MISO,
    .mosi = SPI3_IOMUX_PIN_NUM_MOSI,
};

#endif // __CONFIG_H__