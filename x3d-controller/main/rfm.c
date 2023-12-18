/**
 * @file rfm.c
 * @author Sven Fabricius (sven.fabricius@livediesel.de)
 * @brief RFM96 / SX1231 setup functions
 * @version 0.1
 * @date 2023-11-15
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_log.h"

#include "soc/spi_pins.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_crc.h"

#include "sx1231.h"
#include "rfm.h"

#define RFM_PIN_NUM_MISO                   VSPI_IOMUX_PIN_NUM_MISO
#define RFM_PIN_NUM_MOSI                   VSPI_IOMUX_PIN_NUM_MOSI
#define RFM_PIN_NUM_CLK                    VSPI_IOMUX_PIN_NUM_CLK
#define RFM_PIN_NUM_CS                     VSPI_IOMUX_PIN_NUM_CS
#define RFM_PIN_NUM_IRQ                    GPIO_NUM_4
#define RFM_SPI_HOST                       SPI3_HOST

static const char *TAG = "RFM";

static QueueHandle_t rfm_evt_queue = NULL;
static sx1231_handle_t sx1231_handle = NULL;
static TaskHandle_t transmit_task_handle = NULL;

extern void x3d_processor(uint8_t * buffer);

static void IRAM_ATTR rfm_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    if (sx1231_get_mode(sx1231_handle) == SX1231_MODE_RECEIVER)
    {
        xQueueSendFromISR(rfm_evt_queue, &gpio_num, NULL);
    }
    else if (sx1231_get_mode(sx1231_handle) == SX1231_MODE_TRANSMITTER && transmit_task_handle != NULL)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveIndexedFromISR(transmit_task_handle, 0, &xHigherPriorityTaskWoken);
        transmit_task_handle = NULL;
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

esp_err_t check_message(uint8_t* buffer)
{
    uint8_t length = buffer[0];
    if (length < 3)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    uint16_t crc = ~esp_crc16_be(~0x0000, buffer, length - sizeof(uint16_t));
    uint16_t rxcrc = buffer[length - 2] << 8 | buffer[length - 1];
    if (rxcrc != crc)
    {
        ESP_LOGE(TAG, "CRC mismatch 0x%04x != 0x%04x", rxcrc, crc);
        return ESP_ERR_INVALID_CRC;
    }

    return ESP_OK;
}

static void rfm_process_task(void* arg)
{
    uint32_t io_num;
    for (;;)
    {
        if (xQueueReceive(rfm_evt_queue, &io_num, portMAX_DELAY))
        {
            uint8_t buffer[65];
            sx1231_get_buffer(sx1231_handle, buffer);
            ESP_ERROR_CHECK(sx1231_receive_begin(sx1231_handle));
            if (check_message(buffer) != ESP_OK)
            {
                continue;
            }
            x3d_processor(buffer);
        }
    }
}

esp_err_t rfm_init(void)
{
    spi_bus_config_t buscfg = {
        .miso_io_num = RFM_PIN_NUM_MISO,
        .mosi_io_num = RFM_PIN_NUM_MOSI,
        .sclk_io_num = RFM_PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 64
    };
    ESP_ERROR_CHECK(spi_bus_initialize(RFM_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 8 * 1000 * 1000,       //Clock out at 10 MHz
        .mode = 0,                               //SPI mode 0
        .command_bits = 8,
        .spics_io_num = RFM_PIN_NUM_CS,          //CS pin
        .queue_size = 1,                         //We want to be able to queue 1 transactions at a time
        .input_delay_ns = 50,
    };
    spi_device_handle_t spi;
    ESP_ERROR_CHECK(spi_bus_add_device(RFM_SPI_HOST, &devcfg, &spi));

    ESP_ERROR_CHECK(sx1231_init(spi, &sx1231_handle));

    // setup the SX1231 for the X3D protocol
    ESP_ERROR_CHECK(sx1231_modulation(sx1231_handle, SX1231_DATA_MODE_PACKET, SX1231_MODULATION_FSK, SX1231_MODULATION_SHAPING_00));
    ESP_ERROR_CHECK(sx1231_bit_rate(sx1231_handle, 40000));
    ESP_ERROR_CHECK(sx1231_fdev(sx1231_handle, 80000));
    ESP_ERROR_CHECK(sx1231_frequency(sx1231_handle, 868950000U));
    ESP_ERROR_CHECK(sx1231_rx_bw(sx1231_handle, SX1231_DCC_CUTOFF_PERCENT_4, SX1231_RX_BW_FSK_KHZ_125DOT0));
    ESP_ERROR_CHECK(sx1231_rx_afc_bw(sx1231_handle, SX1231_DCC_CUTOFF_PERCENT_1, SX1231_RX_BW_FSK_KHZ_41DOT7));
    ESP_ERROR_CHECK(sx1231_afc_fei(sx1231_handle, false, true, true, true, false));
    ESP_ERROR_CHECK(sx1231_dio_mapping(sx1231_handle, SX1231_DIO_PIN_0, SX1231_DIO_TYPE_00, SX1231_DIO_MODE_TX));
    ESP_ERROR_CHECK(sx1231_dio_mapping(sx1231_handle, SX1231_DIO_PIN_0, SX1231_DIO_TYPE_01, SX1231_DIO_MODE_RX));
    ESP_ERROR_CHECK(sx1231_rssi_threshold(sx1231_handle, 114*2));
    ESP_ERROR_CHECK(sx1231_preamble(sx1231_handle, 4));
    uint8_t sync[] = {0x81, 0x69, 0x96, 0x7e};
    ESP_ERROR_CHECK(sx1231_sync(sx1231_handle, true, false, sizeof(sync), 0, sync));
    ESP_ERROR_CHECK(sx1231_packet(sx1231_handle, SX_1231_PACKET_FORMAT_VARIABLE, SX_1231_PACKET_DC_WHITENING, 64, false, false, SX_1231_PACKET_FILTERING_NONE, SX_1231_INTER_PACKET_RX_DELAY_32_BITS, true, false));
    ESP_ERROR_CHECK(sx1231_fifo_threshold(sx1231_handle, true, 15));
    ESP_ERROR_CHECK(sx1231_sensitivity_boost(sx1231_handle, SX1231_SENSITIVITY_BOOST_HIGH_SENSITIVITY));
    ESP_ERROR_CHECK(sx1231_continuous_dagc(sx1231_handle, SX1231_CONTINUOUS_DAGC_IMPROVED_MARGIN_AFC_LOW_BETA_ON_0));
    ESP_ERROR_CHECK(sx1231_mode(sx1231_handle, SX1231_MODE_STANDBY));
    ESP_ERROR_CHECK(sx1231_pa_level(sx1231_handle, false, true, true, 23));

    // setup the DIO0 IRQ for payload processing
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1<<RFM_PIN_NUM_IRQ,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_intr_type(RFM_PIN_NUM_IRQ, GPIO_INTR_POSEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(RFM_PIN_NUM_IRQ, rfm_isr_handler, (void*) RFM_PIN_NUM_IRQ);
    rfm_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(rfm_process_task, "rfm_process_task", 2048, NULL, 5, NULL);

    // start in receiver mode
    return sx1231_receive_begin(sx1231_handle);
}

esp_err_t rfm_receive(void)
{
    return sx1231_receive_begin(sx1231_handle);
}

esp_err_t rfm_transfer(uint8_t * buffer, size_t size)
{
    transmit_task_handle = xTaskGetCurrentTaskHandle();
    esp_err_t res = sx1231_transmit(sx1231_handle, buffer, size);
    ulTaskNotifyTakeIndexed(0, pdFALSE, portMAX_DELAY);
    sx1231_mode(sx1231_handle, SX1231_MODE_STANDBY);
    return res;
}