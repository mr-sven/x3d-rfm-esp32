#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_timer.h"

#include "sx1231.h"
#include "sx1231_register.h"

#define F_SCALE      1000000.0
#define FOSC         (32000000.0 * F_SCALE)
#define FSTEP        (FOSC / 524288.0) // FOSC/2^19

typedef struct {
    sx1231_dio_pin_t pin;
    sx1231_dio_type_t dio_type_rx;
    sx1231_dio_type_t dio_type_tx;
    sx1231_dio_mode_t dio_mode;
} sx1231_dio_mapping_t;

struct sx1231_context_t {
    spi_device_handle_t spi;    ///< SPI device handle
    bool sequencerOff;
    sx1231_mode_t mode;
    sx1231_dio_mapping_t dio[SX1231_DIO_MAPPING_COUNT];
};

typedef struct sx1231_context_t sx1231_context_t;
SemaphoreHandle_t spi_semphr;

uint32_t millis()
{
    return (uint32_t) (esp_timer_get_time() / 1000ULL);
}

int dio_pin_to_index(sx1231_dio_pin_t dio_pin)
{
    switch (dio_pin)
    {
        case SX1231_DIO_PIN_0: return 0;
        case SX1231_DIO_PIN_1: return 1;
        case SX1231_DIO_PIN_2: return 2;
        case SX1231_DIO_PIN_3: return 3;
        case SX1231_DIO_PIN_4: return 4;
        case SX1231_DIO_PIN_5: return 5;
    }
    return 0;
}

bool is_dio_mode_match(sx1231_mode_t mode, sx1231_dio_mode_t dio_mode)
{
    switch (dio_mode)
    {
        case SX1231_DIO_MODE_NONE: break;
        case SX1231_DIO_MODE_RX: if (mode == SX1231_MODE_RECEIVER) { return true; } break;
        case SX1231_DIO_MODE_TX: if (mode == SX1231_MODE_TRANSMITTER) { return true; } break;
        case SX1231_DIO_MODE_BOTH: if (mode == SX1231_MODE_TRANSMITTER || mode == SX1231_MODE_RECEIVER) { return true; } break;
    }
    return false;
}

esp_err_t writeReadSpi(spi_device_handle_t spi, spi_transaction_t *trans)
{
	xSemaphoreTake(spi_semphr, portMAX_DELAY);
    esp_err_t res = spi_device_polling_transmit(spi, trans);
    xSemaphoreGive(spi_semphr);
    return res;
}

esp_err_t writeReg(spi_device_handle_t spi, sx1231_register_t reg, uint8_t value)
{
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.cmd = (uint8_t)reg | SX1231_REGISTER_WRITE;
    t.flags = SPI_TRANS_USE_TXDATA;
    *((uint8_t*)t.tx_data) = SPI_SWAP_DATA_TX(value, 8);
    return writeReadSpi(spi, &t);
}

esp_err_t writeReg16(spi_device_handle_t spi, sx1231_register_t reg, uint16_t value)
{
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 16;
    t.cmd = (uint8_t)reg | SX1231_REGISTER_WRITE;
    t.flags = SPI_TRANS_USE_TXDATA;
    *((uint16_t*)t.tx_data) = SPI_SWAP_DATA_TX(value, 16);
    return writeReadSpi(spi, &t);
}

esp_err_t writeReg24(spi_device_handle_t spi, sx1231_register_t reg, uint32_t value)
{
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 24;
    t.cmd = (uint8_t)reg | SX1231_REGISTER_WRITE;
    t.flags = SPI_TRANS_USE_TXDATA;
    *((uint32_t*)t.tx_data) = SPI_SWAP_DATA_TX(value, 24);
    return writeReadSpi(spi, &t);
}

esp_err_t writeRegBuf(spi_device_handle_t spi, sx1231_register_t reg, uint8_t * value, size_t length)
{
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = length * 8;
    t.cmd = (uint8_t)reg | SX1231_REGISTER_WRITE;
    t.tx_buffer = value;
    return writeReadSpi(spi, &t);
}

uint8_t readReg(spi_device_handle_t spi, sx1231_register_t reg)
{
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.cmd = (uint8_t)reg & SX1231_REGISTER_READ;
    t.flags = SPI_TRANS_USE_RXDATA;
    writeReadSpi(spi, &t);
    return *(uint8_t*)t.rx_data;
}

esp_err_t readRegBuf(spi_device_handle_t spi, sx1231_register_t reg, uint8_t * buffer, size_t length)
{
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = length * 8;
    t.cmd = (uint8_t)reg & SX1231_REGISTER_READ;
    t.tx_buffer = buffer;
    t.rx_buffer = buffer;
    return writeReadSpi(spi, &t);
}

esp_err_t update_dio(sx1231_context_t* ctx)
{
    uint16_t data = SX1231_CLK_OUT_OFF;
    for (int i = 0; i < SX1231_DIO_MAPPING_COUNT; i++)
    {
        if (is_dio_mode_match(ctx->mode, ctx->dio[i].dio_mode))
        {
            if (ctx->mode == SX1231_MODE_RECEIVER)
            {
                data |= ctx->dio[i].dio_type_rx << ctx->dio[i].pin;
            }
            else if (ctx->mode == SX1231_MODE_TRANSMITTER)
            {
                data |= ctx->dio[i].dio_type_tx << ctx->dio[i].pin;
            }
        }
    }
    return writeReg16(ctx->spi, SX1231_REG_DIO_MAPPING_1, data);
}

esp_err_t sx1231_init(spi_device_handle_t spi, sx1231_context_t** out_ctx)
{
    sx1231_context_t* ctx = (sx1231_context_t*)malloc(sizeof(sx1231_context_t));
    if (!ctx)
    {
        return ESP_ERR_NO_MEM;
    }

    *ctx = (sx1231_context_t) {
        .spi = spi,
        .sequencerOff = false,
        .mode = SX1231_MODE_STANDBY,
        .dio = {
            { .dio_mode = SX1231_DIO_MODE_NONE },
            { .dio_mode = SX1231_DIO_MODE_NONE },
            { .dio_mode = SX1231_DIO_MODE_NONE },
            { .dio_mode = SX1231_DIO_MODE_NONE },
            { .dio_mode = SX1231_DIO_MODE_NONE },
            { .dio_mode = SX1231_DIO_MODE_NONE }
        }
    };
    spi_semphr = xSemaphoreCreateMutex();

    uint32_t start = millis();
    uint8_t timeout = 50;
    do
    {
        writeReg(ctx->spi, SX1231_REG_SYNC_VALUE_1, 0xAA);
    } while (readReg(ctx->spi, SX1231_REG_SYNC_VALUE_1) != 0xaa && millis() - start < timeout);
    start = millis();
    do
    {
        writeReg(ctx->spi, SX1231_REG_SYNC_VALUE_1, 0x55);
    } while (readReg(ctx->spi, SX1231_REG_SYNC_VALUE_1) != 0x55 && millis() - start < timeout);

    sx1231_ocp(ctx, false, 15);
    sx1231_mode(ctx, SX1231_MODE_STANDBY);

    start = millis();
    while (((readReg(ctx->spi, SX1231_REG_IRQ_FLAGS_1) & SX1231_IRQ1_MODE_READY) == 0) && millis() - start < timeout); // wait for ModeReady

    if (millis() - start >= timeout)
    {
        return ESP_ERR_TIMEOUT;
    }
    *out_ctx = ctx;
    return ESP_OK;
}

esp_err_t sx1231_ocp(sx1231_context_t* ctx, bool on, uint8_t trim)
{
    return writeReg(ctx->spi, SX1231_REG_OCP, (trim & 0x0F) | (on << 4));
}

esp_err_t sx1231_mode(sx1231_context_t* ctx, sx1231_mode_t mode)
{
    if (ctx->mode == mode)
    {
        return ESP_OK;
    }
    ctx->mode = mode;
    update_dio(ctx);
    return writeReg(ctx->spi, SX1231_REG_OP_MODE, mode | (ctx->sequencerOff << 7));
}

esp_err_t sx1231_modulation(sx1231_context_t* ctx, sx1231_data_mode_t data_mode, sx1231_modulation_type_t mode_type, sx1231_modulation_shaping_t shaping)
{
    return writeReg(ctx->spi, SX1231_REG_DATA_MODUL, data_mode | mode_type | shaping);
}

esp_err_t sx1231_bit_rate(sx1231_context_t* ctx, uint32_t bit_rate)
{
    uint16_t data = (uint16_t)round(FOSC / (F_SCALE * (double)bit_rate));
    return writeReg16(ctx->spi, SX1231_REG_BITRATE_MSB, data);
}

esp_err_t sx1231_fdev(sx1231_context_t* ctx, uint32_t fdev)
{
    uint16_t data = (uint16_t)round((F_SCALE * (double)fdev) / FSTEP);
    return writeReg16(ctx->spi, SX1231_REG_FDEV_MSB, data);
}

esp_err_t sx1231_frequency(sx1231_context_t* ctx, uint32_t frequency)
{
    uint32_t data = (uint32_t)round((F_SCALE * (double)frequency) / FSTEP);
    return writeReg24(ctx->spi, SX1231_REG_FRF_MSB, data);
}

esp_err_t sx1231_rx_bw(sx1231_context_t* ctx, sx1231_dcc_cutoff_t dcc_cutoff, uint8_t rx_bw)
{
    return writeReg(ctx->spi, SX1231_REG_RX_BW, dcc_cutoff | rx_bw);
}

esp_err_t sx1231_rx_afc_bw(sx1231_context_t* ctx, sx1231_dcc_cutoff_t dcc_cutoff, uint8_t rx_bw)
{
    return writeReg(ctx->spi, SX1231_REG_AFC_BW, dcc_cutoff | rx_bw);
}

esp_err_t sx1231_afc_fei(sx1231_context_t* ctx, bool fei_start, bool autoclear_on, bool auto_on, bool clear, bool start)
{
    return writeReg16(ctx->spi, SX1231_REG_AFC_FEI, fei_start << 5 | autoclear_on << 3 | auto_on << 2 | clear << 2 | start);
}

esp_err_t sx1231_dio_mapping(sx1231_context_t* ctx, sx1231_dio_pin_t pin, sx1231_dio_type_t type, sx1231_dio_mode_t mode)
{
    ctx->dio[dio_pin_to_index(pin)].pin = pin;
    if (mode == SX1231_DIO_MODE_TX || mode == SX1231_DIO_MODE_BOTH)
    {
        ctx->dio[dio_pin_to_index(pin)].dio_type_tx = type;
        if (ctx->dio[dio_pin_to_index(pin)].dio_mode == SX1231_DIO_MODE_RX)
        {
            ctx->dio[dio_pin_to_index(pin)].dio_mode = SX1231_DIO_MODE_BOTH;
        }
        else
        {
            ctx->dio[dio_pin_to_index(pin)].dio_mode = SX1231_DIO_MODE_TX;
        }
    }
    if (mode == SX1231_DIO_MODE_RX || mode == SX1231_DIO_MODE_BOTH)
    {
        ctx->dio[dio_pin_to_index(pin)].dio_type_rx = type;
        if (ctx->dio[dio_pin_to_index(pin)].dio_mode == SX1231_DIO_MODE_TX)
        {
            ctx->dio[dio_pin_to_index(pin)].dio_mode = SX1231_DIO_MODE_BOTH;
        }
        else
        {
            ctx->dio[dio_pin_to_index(pin)].dio_mode = SX1231_DIO_MODE_RX;
        }
    }
    return update_dio(ctx);
}

esp_err_t sx1231_rssi_threshold(sx1231_context_t* ctx, uint8_t rssi_threshold)
{
    return writeReg(ctx->spi, SX1231_REG_RSSI_THRESH, rssi_threshold);
}

esp_err_t sx1231_preamble(sx1231_context_t* ctx, uint16_t length)
{
    return writeReg16(ctx->spi, SX1231_REG_PREAMBLE_MSB, length);
}

esp_err_t sx1231_sync(sx1231_context_t* ctx, bool sync_on, bool fifo_fill_condition, uint8_t sync_size, uint8_t sync_tol, uint8_t *sync)
{
    writeReg(ctx->spi, SX1231_REG_SYNC_CONFIG, sync_on << 7 | fifo_fill_condition << 6 | ((sync_size - 1) & 0x07) << 3 | (sync_tol & 0x07));
    return writeRegBuf(ctx->spi, SX1231_REG_SYNC_VALUE_1, sync, sync_size);
}

esp_err_t sx1231_packet(sx1231_context_t* ctx, sx_1231_packet_format_t format, sx_1231_packet_dc_t dc_free, uint8_t payload_length, bool crc_on, bool crc_auto_clear_off, sx_1231_packet_filtering_t filtering, sx_1231_inter_packet_rx_delay_t inter_packet_rx_delay, bool auto_rx_restart, bool aes)
{
    writeReg(ctx->spi, SX1231_REG_PACKET_CONFIG_1, format | dc_free | crc_on << 4 | crc_auto_clear_off << 3 | filtering);
    writeReg(ctx->spi, SX1231_REG_PAYLOAD_LENGTH, payload_length);
    return writeReg(ctx->spi, SX1231_REG_PACKET_CONFIG_2, inter_packet_rx_delay | auto_rx_restart << 1 | aes);
}

esp_err_t sx1231_fifo_threshold(sx1231_context_t* ctx, bool fifo_not_empty, uint8_t threshold)
{
    threshold &= 0x7f;
    threshold |= fifo_not_empty << 7;
    return writeReg(ctx->spi, SX1231_REG_FIFO_THRESH, threshold);
}

esp_err_t sx1231_sensitivity_boost(sx1231_context_t* ctx, sx1231_sensitivity_boost_t boost)
{
    return writeReg(ctx->spi, SX1231_REG_TEST_LNA, boost);
}

esp_err_t sx1231_continuous_dagc(sx1231_context_t* ctx, sx1231_continuous_dagc_t dagc)
{
    return writeReg(ctx->spi, SX1231_REG_TEST_DAGC, dagc);
}

esp_err_t sx1231_pa_level(sx1231_context_t* ctx, bool pa0_on, bool pa1_on, bool pa2_on, uint8_t output_power)
{
    output_power &= 0x1F;
    output_power |= pa0_on << 7;
    output_power |= pa1_on << 6;
    output_power |= pa2_on << 5;
    return writeReg(ctx->spi, SX1231_REG_PA_LEVEL, output_power);
}

esp_err_t sx1231_receive_begin(sx1231_context_t* ctx)
{
    sx1231_mode(ctx, SX1231_MODE_STANDBY);
    return sx1231_mode(ctx, SX1231_MODE_RECEIVER);
}

esp_err_t sx1231_get_buffer(sx1231_context_t* ctx, uint8_t *buffer)
{
    if ((readReg(ctx->spi, SX1231_REG_IRQ_FLAGS_2) & SX1231_IRQ2_PAYLOAD_READY) == 0)
    {
        return 0;
    }
    sx1231_mode(ctx, SX1231_MODE_STANDBY);
    buffer[0] = readReg(ctx->spi, SX1231_REG_FIFO);
    return readRegBuf(ctx->spi, SX1231_REG_FIFO, &buffer[1], buffer[0]);
}

esp_err_t sx1231_transmit(sx1231_context_t* ctx, uint8_t *buffer, size_t size)
{
    sx1231_mode(ctx, SX1231_MODE_STANDBY);

    uint32_t start = millis();
    uint8_t timeout = 50;
    while (((readReg(ctx->spi, SX1231_REG_IRQ_FLAGS_1) & SX1231_IRQ1_MODE_READY) == 0) && millis() - start < timeout); // wait for ModeReady
    if (millis() - start >= timeout)
    {
        return ESP_ERR_TIMEOUT;
    }

    writeRegBuf(ctx->spi, SX1231_REG_FIFO, buffer, size);
    return sx1231_mode(ctx, SX1231_MODE_TRANSMITTER);
}

sx1231_mode_t sx1231_get_mode(sx1231_context_t* ctx)
{
    return ctx->mode;
}