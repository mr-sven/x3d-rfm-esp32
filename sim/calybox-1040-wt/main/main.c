/**
 * @file main.c
 * @author Sven Fabricius (sven.fabricius@livediesel.de)
 * @brief Direct IDF Implementation of X3D ESP32 Gateway
 * @version 0.1
 * @date 2023-11-15
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_log.h"

#include "rfm.h"
#include "x3d.h"

#define MODE_NONE 0
#define MODE_PAIR 1

static const char TAG[] = "MAIN";

x3d_context_t x3d_ctx = {
        .device_id = 0xe44dce,
        .magic_id  = 1,
        .msg_no    = 1,
};

uint8_t x3d_buffer[64];
uint8_t x3d_tx_buffer[64];
TickType_t x3d_last_rx_ts;

TaskHandle_t processing_task_handle = NULL;

/**
 * @brief Wrapper for xTaskCreate to check if a processing task is already executed
 *
 * @param pxTaskCode
 * @param pcName
 * @param usStackDepth
 * @param pvParameters
 */
void execute_task(TaskFunction_t pxTaskCode, const char *const pcName, const configSTACK_DEPTH_TYPE usStackDepth, void *const pvParameters)
{
    if (processing_task_handle != NULL)
    {
        ESP_LOGE(TAG, "X3D message processing in progress");
        return;
    }
    ESP_LOGI(TAG, "Start: %s", pcName);
    xTaskCreate(pxTaskCode, pcName, usStackDepth, pvParameters, 10, &processing_task_handle);
}

/**
 * @brief Task end funktion.
 * Set status to idle, optional frees arg pointer and deletes task.
 *
 * @param arg optional pointer to data to free
 */
static void __attribute__((noreturn)) end_task(void *arg)
{
    if (arg != NULL)
    {
        free(arg);
    }
    processing_task_handle = NULL;
    vTaskDelete(NULL);
    while (1)
        ; // should not be reached
}

/**
 * @brief Task end funktion.
 * Set status to idle, optional frees arg pointer and deletes task.
 *
 * @param arg optional pointer to data to free
 */
static void __attribute__((noreturn)) next_task(TaskFunction_t pxTaskCode, const char *const pcName, const configSTACK_DEPTH_TYPE usStackDepth, void *const pvParameters, void *arg)
{
    if (arg != NULL)
    {
        free(arg);
    }
    processing_task_handle = NULL;
    execute_task(pxTaskCode, pcName, usStackDepth, pvParameters);
    vTaskDelete(NULL);
    while (1)
        ; // should not be reached
}

void x3d_processor(uint8_t *buffer)
{
    // store last rx time to check if air is free.
    x3d_last_rx_ts = xTaskGetTickCount();

    // ESP_LOG_BUFFER_HEX_LEVEL(TAG, buffer, buffer[0], ESP_LOG_INFO);

    memcpy(x3d_buffer, buffer, buffer[0]);
    if (processing_task_handle != NULL)
    {
        xTaskNotifyGive(processing_task_handle);
    }
}

void x3d_transmit(void)
{
    // TODO: check x3d_last_rx_ts for free air
    TickType_t last_send_time = xTaskGetTickCount();
    do
    {
        // ESP_LOG_BUFFER_HEX_LEVEL(TAG, x3d_tx_buffer, x3d_tx_buffer[0], ESP_LOG_INFO);
        vTaskDelayUntil(&last_send_time, pdMS_TO_TICKS(X3D_MSG_DELAY_MS));
        rfm_transfer(x3d_tx_buffer, x3d_tx_buffer[0]);
    } while (x3d_proto_dec_retry(x3d_tx_buffer) > 0);
    rfm_receive();
}

static void device_data_task(void *arg)
{
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        // ESP_LOG_BUFFER_HEX_LEVEL(TAG, x3d_buffer, x3d_buffer[0], ESP_LOG_INFO);
        x3d_pped_register_t reg_data;
        if (x3d_pped_parse_register_req(x3d_buffer, &x3d_ctx, &reg_data) != 0)
        {
            continue;
        }

        reg_data.retry_cnt++;
        int delay                 = reg_data.retry_cnt * X3D_MSG_DELAY_MS;
        TickType_t last_send_time = xTaskGetTickCount();

        if (reg_data.peer_device_id != 0x68d9d7)
        {
            continue;
        }

        ESP_LOGI(TAG, "Read reg: %d pl: %d", reg_data.reg, reg_data.reg_payload_length);

        bool response = false;
        switch (reg_data.reg)
        {

        case 16:
            if (reg_data.reg_payload_length == 0)
            {
                uint8_t data[]              = {0x30, 0x30, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};
                reg_data.reg_payload_length = sizeof(data);
                memcpy(reg_data.reg_payload, data, reg_data.reg_payload_length);
                response = true;
            }
            break;

        case 17:
            if (reg_data.reg_payload_length == 2)
            {
                uint16_t sub_reg;
                x3d_read_le_u16(&sub_reg, reg_data.reg_payload, 0);
                if (sub_reg == 0xffff)
                {
                    uint8_t data[]              = {0x03, 0x00, 0x01, 0xe5, 0x01, 0x05};
                    reg_data.reg_payload_length = sizeof(data);
                    memcpy(reg_data.reg_payload, data, reg_data.reg_payload_length);
                    response = true;
                }
            }
            break;

        case 273:
            if (reg_data.reg_payload_length == 2)
            {
                uint16_t sub_reg;
                x3d_read_le_u16(&sub_reg, reg_data.reg_payload, 0);
                if (sub_reg == 1)
                {
                    uint8_t data[]              = {0x01, 0x00, 0xf6, 0x22};
                    reg_data.reg_payload_length = sizeof(data);
                    memcpy(reg_data.reg_payload, data, reg_data.reg_payload_length);
                    response = true;
                }
                else if (sub_reg == 2)
                {
                    uint8_t data[]              = {0x02, 0x00, 0xf6, 0x27, 0xf6, 0x22};
                    reg_data.reg_payload_length = sizeof(data);
                    memcpy(reg_data.reg_payload, data, reg_data.reg_payload_length);
                    response = true;
                }
            }
            break;

        case 274:
            if (reg_data.reg_payload_length == 2)
            {
                uint16_t sub_reg;
                x3d_read_le_u16(&sub_reg, reg_data.reg_payload, 0);
                if (sub_reg == 1)
                {
                    reg_data.reg_payload_length = 4;
                    int data_p                  = x3d_write_le_u16(1, reg_data.reg_payload, 0);
                    data_p                      = x3d_write_le_u16(2076, reg_data.reg_payload, data_p);
                    response                    = true;
                }
                else if (sub_reg == 2)
                {
                    reg_data.reg_payload_length = 6;
                    int data_p                  = x3d_write_le_u16(1, reg_data.reg_payload, 0);
                    data_p                      = x3d_write_le_u16(0x8000, reg_data.reg_payload, data_p);
                    data_p                      = x3d_write_le_u16(2076, reg_data.reg_payload, data_p);
                    response                    = true;
                }
            }
            break;
        case 528:
            if (reg_data.reg_payload_length == 3)
            {
                uint32_t date_req;
                x3d_read_le_u24(&date_req, reg_data.reg_payload, 0);

                if (date_req == 0xf00400)
                {
                    reg_data.reg_payload_length = 3 + 3 + (8 * 4);
                    int data_p                  = x3d_write_le_u24(0x100400, reg_data.reg_payload, 0);
                    data_p                      = x3d_write_le_u24(0x1a166b, reg_data.reg_payload, data_p);
                    data_p                      = x3d_write_le_u32(90000, reg_data.reg_payload, data_p);      // month 9
                    data_p                      = x3d_write_le_u32(339381, reg_data.reg_payload, data_p);     // month 10
                    data_p                      = x3d_write_le_u32(209161, reg_data.reg_payload, data_p);     // month 11
                    data_p                      = x3d_write_le_u32(120000, reg_data.reg_payload, data_p);     // month 12
                    data_p                      = x3d_write_le_u32(0xffffffff, reg_data.reg_payload, data_p); // ff invalid
                    data_p                      = x3d_write_le_u32(0xffffffff, reg_data.reg_payload, data_p); // ff invalid
                    data_p                      = x3d_write_le_u32(0xffffffff, reg_data.reg_payload, data_p); // ff invalid
                    data_p                      = x3d_write_le_u32(0xffffffff, reg_data.reg_payload, data_p); // ff invalid
                    response                    = true;
                }
                else
                {
                    /*
                    int data_p = x3d_write_le_u24(0x100400, reg_data.reg_payload, 0);
                    data_p     = x3d_write_le_u24(0x1a166b, reg_data.reg_payload, data_p);
                    data_p     = x3d_write_le_u32(10000, reg_data.reg_payload, data_p); // month 1
                    data_p     = x3d_write_le_u32(20000, reg_data.reg_payload, data_p); // month 2
                    data_p     = x3d_write_le_u32(30000, reg_data.reg_payload, data_p); // month 3
                    data_p     = x3d_write_le_u32(40000, reg_data.reg_payload, data_p); // month 4
                    data_p     = x3d_write_le_u32(50000, reg_data.reg_payload, data_p); // month 5
                    data_p     = x3d_write_le_u32(60000, reg_data.reg_payload, data_p); // month 6
                    data_p     = x3d_write_le_u32(70000, reg_data.reg_payload, data_p); // month 7
                    data_p     = x3d_write_le_u32(80000, reg_data.reg_payload, data_p); // month 8
                    */
                }
            }
            break;

        case 784:
            if (reg_data.reg_payload_length == 3)
            {
                x3d_pped_month_consumtion_detail_t month_consumtion_detail = {
                        .month     = 11,
                        .month_f   = 6,
                        .day       = 22,
                        .year      = 26,
                        .heating   = 85402,
                        .cooling   = 0xffffffff,
                        .hot_water = 40580,
                        .outlets   = 49730,
                        .other     = 33449,
                };

                reg_data.reg_payload_length = sizeof(month_consumtion_detail) + 3;
                reg_data.reg_payload[0]     = 0;
                reg_data.reg_payload[1]     = 4;
                reg_data.reg_payload[2]     = 0;
                memcpy(&reg_data.reg_payload[3], &month_consumtion_detail, reg_data.reg_payload_length);
                response = true;
            }
            break;

        case 1041:
            if (reg_data.reg_payload_length == 2)
            {
                uint16_t sub_reg;
                x3d_read_le_u16(&sub_reg, reg_data.reg_payload, 0);
                if (sub_reg == 1)
                {
                    uint8_t data[]              = {0x01, 0x00, 0x39, 0x07};
                    reg_data.reg_payload_length = sizeof(data);
                    memcpy(reg_data.reg_payload, data, reg_data.reg_payload_length);
                    response = true;
                }
                else if (sub_reg == 2)
                {
                    uint8_t data[]              = {0x02, 0x00, 0x3f, 0x07, 0x39, 0x07};
                    reg_data.reg_payload_length = sizeof(data);
                    memcpy(reg_data.reg_payload, data, reg_data.reg_payload_length);
                    response = true;
                }
            }
            break;
        default:
            break;
        }

        if (response)
        {
            x3d_pped_register_resp(x3d_tx_buffer, &x3d_ctx, &reg_data);
            vTaskDelayUntil(&last_send_time, pdMS_TO_TICKS(delay));
            x3d_transmit();
        }
        else
        {
            if (reg_data.reg_payload_length > 0)
            {
                ESP_LOG_BUFFER_HEX_LEVEL(TAG, reg_data.reg_payload, reg_data.reg_payload_length, ESP_LOG_INFO);
            }
        }
    }
    end_task(arg);
}

static void device_pairing_task(void *arg)
{
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        uint8_t retry_cnt;
        uint32_t peer_device_id;
        uint16_t peer_magic_id;

        if (x3d_pped_parse_pairing_req(x3d_buffer, &peer_device_id, &retry_cnt, &peer_magic_id) != 0)
        {
            continue;
        }

        retry_cnt++;
        int delay                 = retry_cnt * X3D_MSG_DELAY_MS;
        TickType_t last_send_time = xTaskGetTickCount();

        x3d_pped_pairing_resp(x3d_tx_buffer, &x3d_ctx, peer_device_id);

        vTaskDelayUntil(&last_send_time, pdMS_TO_TICKS(delay));
        x3d_transmit();

        next_task(device_data_task, "device_data_task", 2048, NULL, arg);
    }
    end_task(arg);
}

void app_main(void)
{
    ESP_ERROR_CHECK(rfm_init());
    vTaskDelay(pdMS_TO_TICKS(5000));
    // execute_task(device_pairing_task, "device_pairing_task", 2048, NULL);
    execute_task(device_data_task, "device_data_task", 2048, NULL);
}
