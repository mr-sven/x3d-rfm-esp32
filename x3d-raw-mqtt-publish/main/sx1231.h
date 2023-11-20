/**
 * @file sx1231.h
 * @author Sven Fabricius (sven.fabricius@livediesel.de)
 * @brief SX1231 implementation
 * @version 0.1
 * @date 2023-11-15
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include "esp_system.h"
#include "driver/spi_master.h"
#include "sx1231_types.h"

/**
 * @brief Handle for SX device, contains SPI handle and some data
 *
 */
typedef struct sx1231_context_t* sx1231_handle_t;

/**
 * @brief inits the SX1231 connection and retuns device handle
 *
 * @param spi SPI device handle
 * @param out_handle SX1231 handle
 * @return esp_err_t
 */
esp_err_t sx1231_init(spi_device_handle_t spi, sx1231_handle_t* out_handle);

/**
 * @brief configures the OCP register
 *
 * @param handle SX1231 handle
 * @param on
 * @param trim
 * @return esp_err_t
 */
esp_err_t sx1231_ocp(sx1231_handle_t handle, bool on, uint8_t trim);
esp_err_t sx1231_mode(sx1231_handle_t handle, sx1231_mode_t mode);
esp_err_t sx1231_modulation(sx1231_handle_t handle, sx1231_data_mode_t data_mode, sx1231_modulation_type_t mode_type, sx1231_modulation_shaping_t shaping);
esp_err_t sx1231_bit_rate(sx1231_handle_t handle, uint32_t bit_rate);
esp_err_t sx1231_fdev(sx1231_handle_t handle, uint32_t fdev);
esp_err_t sx1231_frequency(sx1231_handle_t handle, uint32_t frequency);
esp_err_t sx1231_rx_bw(sx1231_handle_t handle, sx1231_dcc_cutoff_t dcc_cutoff, uint8_t rx_bw);
esp_err_t sx1231_rx_afc_bw(sx1231_handle_t handle, sx1231_dcc_cutoff_t dcc_cutoff, uint8_t rx_bw);
esp_err_t sx1231_afc_fei(sx1231_handle_t handle, bool fei_start, bool autoclear_on, bool auto_on, bool clear, bool start);
esp_err_t sx1231_dio_mapping(sx1231_handle_t handle, sx1231_dio_pin_t pin, sx1231_dio_type_t type, sx1231_dio_mode_t mode);
esp_err_t sx1231_rssi_threshold(sx1231_handle_t handle, uint8_t rssi_threshold);
esp_err_t sx1231_preamble(sx1231_handle_t handle, uint16_t length);
esp_err_t sx1231_sync(sx1231_handle_t handle, bool sync_on, bool fifo_fill_condition, uint8_t sync_size, uint8_t sync_tol, uint8_t *sync);
esp_err_t sx1231_packet(sx1231_handle_t handle, sx_1231_packet_format_t format, sx_1231_packet_dc_t dc_free, uint8_t payload_length, bool crc_on, bool crc_auto_clear_off, sx_1231_packet_filtering_t filtering, sx_1231_inter_packet_rx_delay_t inter_packet_rx_delay, bool auto_rx_restart, bool aes);
esp_err_t sx1231_fifo_threshold(sx1231_handle_t handle, bool fifo_not_empty, uint8_t threshold);
esp_err_t sx1231_sensitivity_boost(sx1231_handle_t handle, sx1231_sensitivity_boost_t boost);
esp_err_t sx1231_continuous_dagc(sx1231_handle_t handle, sx1231_continuous_dagc_t dagc);
esp_err_t sx1231_pa_level(sx1231_handle_t handle, bool pa0_on, bool pa1_on, bool pa2_on, uint8_t output_power);
esp_err_t sx1231_receive_begin(sx1231_handle_t handle);
esp_err_t sx1231_get_buffer(sx1231_handle_t handle, uint8_t *buffer);
esp_err_t sx1231_transmit(sx1231_handle_t handle, uint8_t *buffer, size_t size);