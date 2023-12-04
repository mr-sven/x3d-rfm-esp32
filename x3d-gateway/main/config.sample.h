/**
 * @file config.h
 * @author Sven Fabricius (sven.fabricius@livediesel.de)
 * @brief App Config sample file
 * @version 0.1
 * @date 2023-11-15
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#define WIFI_SSID                          "<your-ssid>"
#define WIFI_PASS                          "<your-wifi-pass>"
#define WIFI_MAXIMUM_RETRY                 5
#define WIFI_SCAN_AUTH_MODE_THRESHOLD      WIFI_AUTH_WPA2_PSK

#define MQTT_BROKER_URL                    "mqtt://<user>:<pass>@<broker>:1883"

#define RFM_PIN_NUM_MISO                   VSPI_IOMUX_PIN_NUM_MISO
#define RFM_PIN_NUM_MOSI                   VSPI_IOMUX_PIN_NUM_MOSI
#define RFM_PIN_NUM_CLK                    VSPI_IOMUX_PIN_NUM_CLK
#define RFM_PIN_NUM_CS                     VSPI_IOMUX_PIN_NUM_CS
#define RFM_PIN_NUM_IRQ                    GPIO_NUM_4
#define RFM_SPI_HOST                       SPI3_HOST

#define LEDC_TIMER                         LEDC_TIMER_0
#define LEDC_MODE                          LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES                      LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_FREQUENCY                     (4000) // Frequency in Hertz. Set frequency at 4 kHz

#define LEDC_RED_OUTPUT                    GPIO_NUM_25
#define LEDC_GREEN_OUTPUT                  GPIO_NUM_32
#define LEDC_BLUE_OUTPUT                   GPIO_NUM_33
#define LEDC_RED_CHANNEL                   LEDC_CHANNEL_0
#define LEDC_GREEN_CHANNEL                 LEDC_CHANNEL_1
#define LEDC_BLUE_CHANNEL                  LEDC_CHANNEL_2