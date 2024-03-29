/**
 * @file sx1231_types.h
 * @author Sven Fabricius (sven.fabricius@livediesel.de)
 * @brief SX1231 Register data types
 * @version 0.1
 * @date 2023-11-15
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

typedef enum {
	SX1231_MODE_SLEEP = (0 << 2),
	SX1231_MODE_STANDBY = (1 << 2),
	SX1231_MODE_FREQUENCY_SYNTHESIZER = (2 << 2),
	SX1231_MODE_TRANSMITTER = (3 << 2),
	SX1231_MODE_RECEIVER = (4 << 2),
	SX1231_MODE_LISTEN_MODE = (1 << 6),
} sx1231_mode_t;

typedef enum {
	SX1231_IRQ1_SYNC_ADDRESS_MATCH = 0x01,
	SX1231_IRQ1_AUTO_MODE = 0x02,
	SX1231_IRQ1_TIMEOUT = 0x04,
	SX1231_IRQ1_RSSI = 0x08,
	SX1231_IRQ1_PLL_LOCK = 0x10,
	SX1231_IRQ1_TX_READY = 0x20,
	SX1231_IRQ1_RX_READY = 0x40,
	SX1231_IRQ1_MODE_READY = 0x80,
} sx1231_irq_flags_1_t;

typedef enum {
	SX1231_IRQ2_CRC_OK = 0x02,
	SX1231_IRQ2_PAYLOAD_READY = 0x04,
	SX1231_IRQ2_PACKET_SENT = 0x08,
	SX1231_IRQ2_FIFO_OVERRUN = 0x10,
	SX1231_IRQ2_FIFO_LEVEL = 0x20,
	SX1231_IRQ2_FIFO_NOT_EMPTY = 0x40,
	SX1231_IRQ2_FIFO_FULL = 0x80,
} sx1231_irq_flags_2_t;

typedef enum {
	SX1231_DATA_MODE_PACKET = 0x00,
	SX1231_DATA_MODE_CONTINUOUS = 0x40,
	SX1231_DATA_MODE_CONTINUOUS_BIT_SYNC = 0x60,
} sx1231_data_mode_t;

typedef enum {
	SX1231_MODULATION_FSK = 0x00,
	SX1231_MODULATION_OOK = 0x08,
} sx1231_modulation_type_t;

typedef enum {
	SX1231_MODULATION_SHAPING_00 = 0x00,
	SX1231_MODULATION_SHAPING_01 = 0x01,
	SX1231_MODULATION_SHAPING_10 = 0x02,
	SX1231_MODULATION_SHAPING_11 = 0x03,
} sx1231_modulation_shaping_t;

typedef enum {
	SX1231_DCC_CUTOFF_PERCENT_16 = 0x00,
	SX1231_DCC_CUTOFF_PERCENT_8 = 0x20,
	SX1231_DCC_CUTOFF_PERCENT_4 = 0x40,
	SX1231_DCC_CUTOFF_PERCENT_2 = 0x60,
	SX1231_DCC_CUTOFF_PERCENT_1 = 0x80,
	SX1231_DCC_CUTOFF_PERCENT_0DOT5 = 0xA0,
	SX1231_DCC_CUTOFF_PERCENT_0DOT25 = 0xC0,
	SX1231_DCC_CUTOFF_PERCENT_0DOT125 = 0xE0,
} sx1231_dcc_cutoff_t;

typedef enum {
	SX1231_RX_BW_FSK_KHZ_2DOT6 = 0b10 << 3 | 7,
	SX1231_RX_BW_FSK_KHZ_3DOT1 = 0b01 << 3 | 7,
	SX1231_RX_BW_FSK_KHZ_3DOT9 = 7,
	SX1231_RX_BW_FSK_KHZ_5DOT2 = 0b10 << 3 | 6,
	SX1231_RX_BW_FSK_KHZ_6DOT3 = 0b01 << 3 | 6,
	SX1231_RX_BW_FSK_KHZ_7DOT8 = 6,
	SX1231_RX_BW_FSK_KHZ_10DOT4 = 0b10 << 3 | 5,
	SX1231_RX_BW_FSK_KHZ_12DOT5 = 0b01 << 3 | 5,
	SX1231_RX_BW_FSK_KHZ_15DOT6 = 5,
	SX1231_RX_BW_FSK_KHZ_20DOT8 = 0b10 << 3 | 4,
	SX1231_RX_BW_FSK_KHZ_25DOT0 = 0b01 << 3 | 4,
	SX1231_RX_BW_FSK_KHZ_31DOT3 = 4,
	SX1231_RX_BW_FSK_KHZ_41DOT7 = 0b10 << 3 | 3,
	SX1231_RX_BW_FSK_KHZ_50DOT0 = 0b01 << 3 | 3,
	SX1231_RX_BW_FSK_KHZ_62DOT5 = 3,
	SX1231_RX_BW_FSK_KHZ_83DOT3 = 0b10 << 3 | 2,
	SX1231_RX_BW_FSK_KHZ_100DOT0 = 0b01 << 3 | 2,
	SX1231_RX_BW_FSK_KHZ_125DOT0 = 2,
	SX1231_RX_BW_FSK_KHZ_166DOT7 = 0b10 << 3 | 1,
	SX1231_RX_BW_FSK_KHZ_200DOT0 = 0b01 << 3 | 1,
	SX1231_RX_BW_FSK_KHZ_250DOT0 = 1,
	SX1231_RX_BW_FSK_KHZ_333DOT3 = 0b10 << 3,
	SX1231_RX_BW_FSK_KHZ_400DOT0 = 0b01 << 3,
	SX1231_RX_BW_FSK_KHZ_500DOT0 = 0,
} sx1231_rx_bw_fsk_t;

typedef enum {
	SX1231_RX_BW_OOK_KHZ_1DOT3 = 0b10 << 3 | 7,
	SX1231_RX_BW_OOK_KHZ_1DOT6 = 0b01 << 3 | 7,
	SX1231_RX_BW_OOK_KHZ_2DOT0 = 7,
	SX1231_RX_BW_OOK_KHZ_2DOT6 = 0b10 << 3 | 6,
	SX1231_RX_BW_OOK_KHZ_3DOT1 = 0b01 << 3 | 6,
	SX1231_RX_BW_OOK_KHZ_3DOT9 = 6,
	SX1231_RX_BW_OOK_KHZ_5DOT2 = 0b10 << 3 | 5,
	SX1231_RX_BW_OOK_KHZ_6DOT3 = 0b01 << 3 | 5,
	SX1231_RX_BW_OOK_KHZ_7DOT8 = 5,
	SX1231_RX_BW_OOK_KHZ_10DOT4 = 0b10 << 3 | 4,
	SX1231_RX_BW_OOK_KHZ_12DOT5 = 0b01 << 3 | 4,
	SX1231_RX_BW_OOK_KHZ_15DOT6 = 4,
	SX1231_RX_BW_OOK_KHZ_20DOT8 = 0b10 << 3 | 3,
	SX1231_RX_BW_OOK_KHZ_25DOT0 = 0b01 << 3 | 3,
	SX1231_RX_BW_OOK_KHZ_31DOT3 = 3,
	SX1231_RX_BW_OOK_KHZ_41DOT7 = 0b10 << 3 | 2,
	SX1231_RX_BW_OOK_KHZ_50DOT0 = 0b01 << 3 | 2,
	SX1231_RX_BW_OOK_KHZ_62DOT5 = 2,
	SX1231_RX_BW_OOK_KHZ_83DOT3 = 0b10 << 3 | 1,
	SX1231_RX_BW_OOK_KHZ_100DOT0 = 0b01 << 3 | 1,
	SX1231_RX_BW_OOK_KHZ_125DOT0 = 1,
	SX1231_RX_BW_OOK_KHZ_166DOT7 = 0b10 << 3,
	SX1231_RX_BW_OOK_KHZ_200DOT0 = 0b01 << 3,
	SX1231_RX_BW_OOK_KHZ_250DOT0 = 0,
} sx1231_rx_bw_ook_t;

typedef enum {
	SX1231_DIO_PIN_0 = 14,
	SX1231_DIO_PIN_1 = 12,
	SX1231_DIO_PIN_2 = 10,
	SX1231_DIO_PIN_3 = 8,
	SX1231_DIO_PIN_4 = 6,
	SX1231_DIO_PIN_5 = 4,
} sx1231_dio_pin_t;

typedef enum {
	SX1231_DIO_TYPE_00 = 0b00,
	SX1231_DIO_TYPE_01 = 0b01,
	SX1231_DIO_TYPE_10 = 0b10,
	SX1231_DIO_TYPE_11 = 0b11,
} sx1231_dio_type_t;

typedef enum {
	SX1231_DIO_MODE_RX,
	SX1231_DIO_MODE_TX,
	SX1231_DIO_MODE_BOTH,
	SX1231_DIO_MODE_NONE,
} sx1231_dio_mode_t;

typedef enum {
	SX1231_CLK_OUT_FXOSC = 0,
	SX1231_CLK_OUT_FXOSC_2 = 1,
	SX1231_CLK_OUT_FXOSC_4 = 2,
	SX1231_CLK_OUT_FXOSC_8 = 3,
	SX1231_CLK_OUT_FXOSC_16 = 4,
	SX1231_CLK_OUT_FXOSC_32 = 5,
	SX1231_CLK_OUT_RC = 6,
	SX1231_CLK_OUT_OFF = 7,
} sx1231_clk_out_t;

typedef enum {
	SX_1231_INTER_PACKET_RX_DELAY_1_BIT = 0x00,
	SX_1231_INTER_PACKET_RX_DELAY_2_BITS = 0x10,
	SX_1231_INTER_PACKET_RX_DELAY_4_BITS = 0x20,
	SX_1231_INTER_PACKET_RX_DELAY_8_BITS = 0x30,
	SX_1231_INTER_PACKET_RX_DELAY_16_BITS = 0x40,
	SX_1231_INTER_PACKET_RX_DELAY_32_BITS = 0x50,
	SX_1231_INTER_PACKET_RX_DELAY_64_BITS = 0x60,
	SX_1231_INTER_PACKET_RX_DELAY_128_BITS = 0x70,
	SX_1231_INTER_PACKET_RX_DELAY_256_BITS = 0x80,
	SX_1231_INTER_PACKET_RX_DELAY_512_BITS = 0x90,
	SX_1231_INTER_PACKET_RX_DELAY_1024_BITS = 0xA0,
	SX_1231_INTER_PACKET_RX_DELAY_2048_BITS = 0xB0,
} sx_1231_inter_packet_rx_delay_t;

typedef enum {
	SX_1231_PACKET_FILTERING_NONE = 0x00,
	SX_1231_PACKET_FILTERING_ADDRESS = 0x02,
	SX_1231_PACKET_FILTERING_BROADCAST = 0x04,
} sx_1231_packet_filtering_t;

typedef enum {
	SX_1231_PACKET_DC_NONE = 0x00,
	SX_1231_PACKET_DC_MANCHESTER = 0x20,
	SX_1231_PACKET_DC_WHITENING = 0x40,
} sx_1231_packet_dc_t;

typedef enum {
	SX_1231_PACKET_FORMAT_VARIABLE = 0x80,
	SX_1231_PACKET_FORMAT_FIXED = 0x00,
} sx_1231_packet_format_t;

typedef enum {
	SX1231_SENSITIVITY_BOOST_NORMAL = 0x1B,
	SX1231_SENSITIVITY_BOOST_HIGH_SENSITIVITY = 0x2D,
} sx1231_sensitivity_boost_t;

typedef enum {
	SX1231_CONTINUOUS_DAGC_NORMAL = 0x00,
	SX1231_CONTINUOUS_DAGC_IMPROVED_MARGIN_AFC_LOW_BETA_ON_1 = 0x20,
	SX1231_CONTINUOUS_DAGC_IMPROVED_MARGIN_AFC_LOW_BETA_ON_0 = 0x30,
} sx1231_continuous_dagc_t;