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

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_timer.h"

#include "nvs_flash.h"
#include "cJSON.h"

#include "config.h"
#include "wifi.h"
#include "rfm.h"
#include "mqtt.h"
#include "led.h"
#include "x3d_handler.h"

#define MQTT_TOPIC_PREFIX_LEN               20
#define MQTT_TOPIC_STATUS                   "/status"
#define MQTT_TOPIC_RESULT                   "/result"

#define MQTT_TOPIC_CMD                      "/cmd"
#define MQTT_CMD_PAIR                       "pair"
#define MQTT_CMD_READ                       "read"
#define MQTT_CMD_WRITE                      "write"
#define MQTT_CMD_UNPAIR                     "unpair"
#define MQTT_CMD_OUTDOOR_TEMP               "outdoorTemp"
#define MQTT_CMD_INFO                       "info"
#define MQTT_CMD_STATUS                     "status"

#define MQTT_STATUS_IDLE                    "idle"
#define MQTT_STATUS_PAIRING                 "pairing"
#define MQTT_STATUS_PAIRING_SUCCESS         "pairing success"
#define MQTT_STATUS_PAIRING_FAILED          "pairing failed"
#define MQTT_STATUS_READING                 "reading"
#define MQTT_STATUS_WRITING                 "writing"
#define MQTT_STATUS_UNPAIRING               "unpairing"
#define MQTT_STATUS_TEMP                    "temp"
#define MQTT_STATUS_STATUS                  "status"

#define MQTT_JSON_ACTION                    "action"
#define MQTT_JSON_NETWORK                   "net"
#define MQTT_JSON_ACK                       "ack"
#define MQTT_JSON_REGISTER_HIGH             "regHigh"
#define MQTT_JSON_REGISTER_LOW              "regLow"
#define MQTT_JSON_VALUES                    "values"
#define MQTT_JSON_TRANSFER                  "transfer"
#define MQTT_JSON_TARGET                    "target"
#define MQTT_JSON_TEMP                      "temp"
#define MQTT_JSON_ROOM_TEMP                 "roomTemp"
#define MQTT_JSON_DEVICE                    "device"
#define MQTT_JSON_SET_POINT                 "setPoint"
#define MQTT_JSON_ENABLED                   "enabled"
#define MQTT_JSON_DEFROST                   "defrost"
#define MQTT_JSON_TIMED                     "timed"
#define MQTT_JSON_HEATER_ON                 "heaterOn"
#define MQTT_JSON_HEATER_STOPPED            "heaterStopped"
#define MQTT_JSON_WINDOW_OPEN               "windowOpen"
#define MQTT_JSON_NO_TEMP_SENSOR            "noTempSensor"
#define MQTT_JSON_BATTERY_LOW               "batteryLow"

#define NVS_NET_4_DEVICE_MASK               "net_4_mask"
#define NVS_NET_5_DEVICE_MASK               "net_5_mask"

#define NET_4                               4
#define NET_5                               5

// 0x16 0x11 Flags
#define X3D_FLAG_DEFROST                    0x0200
#define X3D_FLAG_TIMED                      0x0800
#define X3D_FLAG_HEATER_ON                  0x1000
#define X3D_FLAG_HEATER_STOPPED             0x2000
#define X3D_FLAG_STATUS                     0x8000

// 0x16 0x21 Flags
#define X3D_FLAG_WINDOW_OPEN                0x0002
#define X3D_FLAG_NO_TEMP_SENSOR             0x0100
#define X3D_FLAG_BATTERY_LOW                0x1000

static const char *TAG = "MAIN";

static char *mqtt_topic_prefix = NULL;
static char *mqtt_status_topic = NULL;
static char *mqtt_result_topic = NULL;

TaskHandle_t x3d_processing_task_handle = NULL;

static uint16_t net_4_device_mask = 0;
static uint16_t net_5_device_mask = 0;

static inline int valid_network(uint8_t network)
{
    return network == NET_4 || network == NET_5;
}

static inline int no_of_devices(uint16_t mask)
{
    return __builtin_popcount(mask);
}

static inline uint16_t get_network_mask(uint8_t network)
{
    switch (network)
    {
        case NET_4:
            return net_4_device_mask;
        case NET_5:
            return net_5_device_mask;
        default:
            return 0;
    }
}

void set_status(char *status)
{
    mqtt_publish(mqtt_status_topic, status, strlen(status), 0, 1);
}

void update_nvs_device_mask(uint8_t network, uint16_t mask)
{
    nvs_handle_t nvs_ctx_handle;
    if (nvs_open("ctx", NVS_READWRITE, &nvs_ctx_handle) == ESP_OK)
    {
        if (network == NET_4)
        {
            net_4_device_mask = mask;
            nvs_set_u16(nvs_ctx_handle, NVS_NET_4_DEVICE_MASK, net_4_device_mask);
        }
        else if (network == NET_5)
        {
            net_5_device_mask = mask;
            nvs_set_u16(nvs_ctx_handle, NVS_NET_5_DEVICE_MASK, net_5_device_mask);
        }
        nvs_commit(nvs_ctx_handle);
        nvs_close(nvs_ctx_handle);
    }
}

void pairing_task(void *arg)
{
    set_status(MQTT_STATUS_PAIRING);
    x3d_pairing_data_t *data = (x3d_pairing_data_t *)arg;
    if (x3d_pairing_proc(data) == X3D_HANDLER_OK)
    {
        set_status(MQTT_STATUS_PAIRING_SUCCESS);
        update_nvs_device_mask(data->network, data->transfer);
    }
    else
    {
        set_status(MQTT_STATUS_PAIRING_FAILED);
    }

    free(data);
    set_status(MQTT_STATUS_IDLE);
    x3d_processing_task_handle = NULL;
    vTaskDelete(NULL);
}

void process_pairing(cJSON *cmd)
{
    cJSON *network = cJSON_GetObjectItem(cmd, MQTT_JSON_NETWORK);
    if (network == NULL || !cJSON_IsNumber(network) || !valid_network(network->valueint))
    {
        return;
    }

    x3d_pairing_data_t *data = (x3d_pairing_data_t *)malloc(sizeof(x3d_pairing_data_t));
    data->network = network->valueint;
    data->transfer = get_network_mask(data->network);

    if (no_of_devices(data->transfer) >= X3D_MAX_NET_DEVICES)
    {
        return;
    }

    if (x3d_processing_task_handle != NULL)
    {
        ESP_LOGE(TAG, "X3D message processing in progress");
        return;
    }
    ESP_LOGI(TAG, "process_pairing network %d", data->network);
    xTaskCreate(pairing_task, "pairing_task", 2048, data, 10, &x3d_processing_task_handle);
}

void unpairing_task(void *arg)
{
    set_status(MQTT_STATUS_UNPAIRING);
    x3d_unpairing_data_t *data = (x3d_unpairing_data_t *)arg;

    x3d_unpairing_proc(data);
    update_nvs_device_mask(data->network, data->transfer);

    free(data);
    set_status(MQTT_STATUS_IDLE);
    x3d_processing_task_handle = NULL;
    vTaskDelete(NULL);
}

void process_unpairing(cJSON *cmd)
{
    cJSON *network = cJSON_GetObjectItem(cmd, MQTT_JSON_NETWORK);
    cJSON *target = cJSON_GetObjectItem(cmd, MQTT_JSON_TARGET);

    if (network == NULL || target == NULL ||
        !cJSON_IsNumber(network) || !cJSON_IsNumber(target) ||
        !valid_network(network->valueint))
    {
        return;
    }

    x3d_unpairing_data_t *data = (x3d_unpairing_data_t *)malloc(sizeof(x3d_unpairing_data_t));
    data->network = network->valueint;
    data->target = (1 << (target->valueint & 0x0f));
    data->transfer = get_network_mask(data->network);

    if (data->transfer == 0 ||
        (data->transfer & data->target) == 0)
    {
        return;
    }

    if (x3d_processing_task_handle != NULL)
    {
        ESP_LOGE(TAG, "X3D message processing in progress");
        return;
    }
    ESP_LOGI(TAG, "process_unpairing network %d via %04x to %04x", data->network, data->transfer, data->target);
    xTaskCreate(unpairing_task, "unpairing_task", 2048, data, 10, &x3d_processing_task_handle);
}

void reading_task(void *arg)
{
    set_status(MQTT_STATUS_READING);
    x3d_read_data_t *data = (x3d_read_data_t *)arg;

    x3d_standard_msg_payload_t *payload = x3d_reading_proc(data);

    ESP_LOGI(TAG, "read register %02x - %02x from %04x", payload->reg_high, payload->reg_low, payload->target_ack);

    int data_slots = ((payload->action & 0xf0) >> 4) + 1;
    cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, MQTT_JSON_ACTION, MQTT_CMD_READ);
	cJSON_AddNumberToObject(root, MQTT_JSON_NETWORK, data->network);
	cJSON_AddNumberToObject(root, MQTT_JSON_ACK, payload->target_ack);
	cJSON_AddNumberToObject(root, MQTT_JSON_REGISTER_HIGH, payload->reg_high);
	cJSON_AddNumberToObject(root, MQTT_JSON_REGISTER_LOW, payload->reg_low);
    cJSON *values = cJSON_AddArrayToObject(root, MQTT_JSON_VALUES);
    for (int i = 0; i < data_slots; i++)
    {
	    cJSON_AddItemToArray(values, cJSON_CreateNumber(payload->data[i]));
    }
    char *json_string = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

    mqtt_publish(mqtt_result_topic, json_string, strlen(json_string), 0, 0);
    free(json_string);

    free(data);
    set_status(MQTT_STATUS_IDLE);
    x3d_processing_task_handle = NULL;
    vTaskDelete(NULL);
}

void process_reading(cJSON *cmd)
{
    cJSON *network = cJSON_GetObjectItem(cmd, MQTT_JSON_NETWORK);
    cJSON *target = cJSON_GetObjectItem(cmd, MQTT_JSON_TARGET);
    cJSON *register_high = cJSON_GetObjectItem(cmd, MQTT_JSON_REGISTER_HIGH);
    cJSON *register_low = cJSON_GetObjectItem(cmd, MQTT_JSON_REGISTER_LOW);
    if (network == NULL || register_high == NULL || register_low == NULL ||
        !cJSON_IsNumber(network) || !cJSON_IsNumber(register_high) || !cJSON_IsNumber(register_low) ||
        !valid_network(network->valueint))
    {
        return;
    }

    x3d_read_data_t *data = (x3d_read_data_t *)malloc(sizeof(x3d_read_data_t));
    data->network = network->valueint;
    data->transfer = get_network_mask(data->network);
    if (target != NULL && cJSON_IsNumber(target))
    {
        data->target = target->valueint;
    }
    else
    {
        data->target = get_network_mask(data->network);
    }
    data->register_high = register_high->valueint;
    data->register_low = register_low->valueint;

    if (data->transfer == 0 ||
        (data->transfer & data->target) == 0)
    {
        return;
    }

    if (x3d_processing_task_handle != NULL)
    {
        ESP_LOGE(TAG, "X3D message processing in progress");
        return;
    }
    ESP_LOGI(TAG, "process_reading network %d via %04x from %04x register %02x - %02x", data->network, data->transfer, data->target, data->register_high, data->register_low);
    xTaskCreate(reading_task, "reading_task", 4096, data, 10, &x3d_processing_task_handle);
}

void writing_task(void *arg)
{
    set_status(MQTT_STATUS_WRITING);
    x3d_write_data_t *data = (x3d_write_data_t *)arg;

    x3d_standard_msg_payload_t *payload = x3d_writing_proc(data);

    ESP_LOGI(TAG, "write register %02x - %02x to %04x", payload->reg_high, payload->reg_low, payload->target_ack);

    int data_slots = ((payload->action & 0xf0) >> 4) + 1;
    cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, MQTT_JSON_ACTION, MQTT_CMD_WRITE);
	cJSON_AddNumberToObject(root, MQTT_JSON_NETWORK, data->network);
	cJSON_AddNumberToObject(root, MQTT_JSON_ACK, payload->target_ack);
	cJSON_AddNumberToObject(root, MQTT_JSON_REGISTER_HIGH, payload->reg_high);
	cJSON_AddNumberToObject(root, MQTT_JSON_REGISTER_LOW, payload->reg_low);
    cJSON *values = cJSON_AddArrayToObject(root, MQTT_JSON_VALUES);
    for (int i = 0; i < data_slots; i++)
    {
	    cJSON_AddItemToArray(values, cJSON_CreateNumber(payload->data[i]));
    }
    char *json_string = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

    mqtt_publish(mqtt_result_topic, json_string, strlen(json_string), 0, 0);
    free(json_string);

    free(data);
    set_status(MQTT_STATUS_IDLE);
    x3d_processing_task_handle = NULL;
    vTaskDelete(NULL);
}

void process_writing(cJSON *cmd)
{
    cJSON *network = cJSON_GetObjectItem(cmd, MQTT_JSON_NETWORK);
    cJSON *target = cJSON_GetObjectItem(cmd, MQTT_JSON_TARGET);
    cJSON *register_high = cJSON_GetObjectItem(cmd, MQTT_JSON_REGISTER_HIGH);
    cJSON *register_low = cJSON_GetObjectItem(cmd, MQTT_JSON_REGISTER_LOW);
    cJSON *values = cJSON_GetObjectItem(cmd, MQTT_JSON_VALUES);

    if (network == NULL || target == NULL || register_high == NULL || register_low == NULL || values == NULL ||
        !cJSON_IsNumber(network) || !cJSON_IsNumber(target) || !cJSON_IsNumber(register_high) || !cJSON_IsNumber(register_low) || cJSON_GetArraySize(values) < 1 ||
        !valid_network(network->valueint))
    {
        return;
    }

    x3d_write_data_t *data = (x3d_write_data_t *)malloc(sizeof(x3d_write_data_t));
    data->network = network->valueint;
    data->transfer = get_network_mask(data->network);
    data->target = target->valueint;
    data->register_high = register_high->valueint;
    data->register_low = register_low->valueint;

    if (data->transfer == 0 ||
        (data->transfer & data->target) == 0)
    {
        return;
    }

    int value_count = cJSON_GetArraySize(values);
    if (value_count == 1)
    {
        cJSON *value = cJSON_GetArrayItem(values, 0);
        if (!cJSON_IsNumber(value))
        {
            return;
        }

        for (int i = 0; i < X3D_MAX_PAYLOAD_DATA_FIELDS; i++)
        {
            data->values[i] = value->valueint;
        }
    }
    else
    {
        for (int i = 0; i < X3D_MAX_PAYLOAD_DATA_FIELDS && i < value_count; i++)
        {
            cJSON *value = cJSON_GetArrayItem(values, i);
            if (!cJSON_IsNumber(value))
            {
                return;
            }
            data->values[i] = value->valueint;
        }
    }

    if (x3d_processing_task_handle != NULL)
    {
        ESP_LOGE(TAG, "X3D message processing in progress");
        return;
    }
    ESP_LOGI(TAG, "process_writing network %d via %04x to %04x register %02x - %02x", data->network, data->transfer, data->target, data->register_high, data->register_low);
    xTaskCreate(writing_task, "writing_task", 4096, data, 10, &x3d_processing_task_handle);
}

void outdoor_temp_task(void *arg)
{
    set_status(MQTT_STATUS_TEMP);
    uint16_t * temp_value = (uint16_t *)arg;
    x3d_temp_data_t data = {
        .room = 0x01,
        .temp = *temp_value
    };

    if (no_of_devices(net_4_device_mask))
    {
        data.network = NET_4;
        data.target = net_4_device_mask;
        data.transfer = net_4_device_mask;
        x3d_temp_proc(&data);
    }

    if (no_of_devices(net_5_device_mask))
    {
        data.network = NET_5;
        data.target = net_5_device_mask;
        data.transfer = net_5_device_mask;
        x3d_temp_proc(&data);
    }

    free(temp_value);
    set_status(MQTT_STATUS_IDLE);
    x3d_processing_task_handle = NULL;
    vTaskDelete(NULL);
}

void process_outdoor_temp(cJSON *cmd)
{
    cJSON *temp = cJSON_GetObjectItem(cmd, MQTT_JSON_TEMP);
    if (temp == NULL || !cJSON_IsNumber(temp))
    {
        return;
    }

    uint16_t * temp_value = (uint16_t *)malloc(sizeof(uint16_t));
    *temp_value = temp->valuedouble * 100.0;

    if (x3d_processing_task_handle != NULL)
    {
        ESP_LOGE(TAG, "X3D message processing in progress");
        return;
    }
    ESP_LOGI(TAG, "process_outdoor_temp network %f °C", temp->valuedouble);
    xTaskCreate(outdoor_temp_task, "outdoor_temp_task", 2048, temp_value, 10, &x3d_processing_task_handle);
}

void process_info(cJSON *cmd)
{
    cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, MQTT_JSON_ACTION, MQTT_CMD_INFO);
	cJSON_AddNumberToObject(root, NVS_NET_4_DEVICE_MASK, net_4_device_mask);
	cJSON_AddNumberToObject(root, NVS_NET_5_DEVICE_MASK, net_5_device_mask);
    char *json_string = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
    mqtt_publish(mqtt_result_topic, json_string, strlen(json_string), 0, 0);
    free(json_string);
}

void init_dev_objects(cJSON **dev_objects, int network, uint16_t mask)
{
    for (int i = 0; i < X3D_MAX_NET_DEVICES; i++)
    {
        if (mask & (1 << i))
        {
            dev_objects[i] = cJSON_CreateObject();
            cJSON_AddNumberToObject(dev_objects[i], MQTT_JSON_NETWORK, network);
            cJSON_AddNumberToObject(dev_objects[i], MQTT_JSON_DEVICE, i);
        }
    }
}

void save_dev_objects(cJSON **dev_objects, cJSON *values)
{
    for (int i = 0; i < X3D_MAX_NET_DEVICES; i++)
    {
        if (dev_objects[i] != NULL)
        {
            cJSON_AddItemToArray(values, dev_objects[i]);
        }
    }
}

void load_register(cJSON **dev_objects, x3d_read_data_t *data)
{
    data->register_high = 0x15;
    data->register_low = 0x11;
    x3d_standard_msg_payload_t *payload = x3d_reading_proc(data);
    for (int i = 0; i < X3D_MAX_NET_DEVICES; i++)
    {
        if (payload->target_ack & (1 << i))
        {
            cJSON_AddNumberToObject(dev_objects[i], MQTT_JSON_ROOM_TEMP, (double)payload->data[i] / 100.0);
        }
    }

    data->register_high = 0x16;
    data->register_low = 0x11;
    payload = x3d_reading_proc(data);
    for (int i = 0; i < X3D_MAX_NET_DEVICES; i++)
    {
        if (payload->target_ack & (1 << i))
        {
            double set_point = payload->data[i] & 0xff;
            uint16_t flags = payload->data[i] & 0xff00;
            cJSON_AddNumberToObject(dev_objects[i], MQTT_JSON_SET_POINT, set_point * 0.5);
            cJSON_AddBoolToObject(dev_objects[i], MQTT_JSON_DEFROST, flags & X3D_FLAG_DEFROST);
            cJSON_AddBoolToObject(dev_objects[i], MQTT_JSON_TIMED, flags & X3D_FLAG_TIMED);
            cJSON_AddBoolToObject(dev_objects[i], MQTT_JSON_HEATER_ON, flags & X3D_FLAG_HEATER_ON);
            cJSON_AddBoolToObject(dev_objects[i], MQTT_JSON_HEATER_STOPPED, flags & X3D_FLAG_HEATER_STOPPED);
        }
    }

    data->register_high = 0x16;
    data->register_low = 0x21;
    payload = x3d_reading_proc(data);
    for (int i = 0; i < X3D_MAX_NET_DEVICES; i++)
    {
        if (payload->target_ack & (1 << i))
        {
            cJSON_AddBoolToObject(dev_objects[i], MQTT_JSON_WINDOW_OPEN, payload->data[i] & X3D_FLAG_WINDOW_OPEN);
            cJSON_AddBoolToObject(dev_objects[i], MQTT_JSON_NO_TEMP_SENSOR, payload->data[i] & X3D_FLAG_NO_TEMP_SENSOR);
            cJSON_AddBoolToObject(dev_objects[i], MQTT_JSON_BATTERY_LOW, payload->data[i] & X3D_FLAG_BATTERY_LOW);
        }
    }

    data->register_high = 0x16;
    data->register_low = 0x41;
    payload = x3d_reading_proc(data);
    for (int i = 0; i < X3D_MAX_NET_DEVICES; i++)
    {
        if (payload->target_ack & (1 << i))
        {
            cJSON_AddBoolToObject(dev_objects[i], MQTT_JSON_ENABLED, payload->data[i] & 0x0001);
        }
    }
}

void process_status_task(void *arg)
{
    set_status(MQTT_STATUS_STATUS);

    cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, MQTT_JSON_ACTION, MQTT_CMD_STATUS);
    cJSON *values = cJSON_AddArrayToObject(root, MQTT_JSON_VALUES);

    if (no_of_devices(net_4_device_mask))
    {
        cJSON *dev_objects[X3D_MAX_NET_DEVICES] = { NULL };
        init_dev_objects(dev_objects, NET_4, net_4_device_mask);
        x3d_read_data_t data = {
            .network = NET_4,
            .transfer = net_4_device_mask,
            .target = net_4_device_mask
        };
        load_register(dev_objects, &data);
        save_dev_objects(dev_objects, values);
    }

    if (no_of_devices(net_5_device_mask))
    {
        cJSON *dev_objects[X3D_MAX_NET_DEVICES] = { NULL };
        init_dev_objects(dev_objects, NET_5, net_5_device_mask);
        x3d_read_data_t data = {
            .network = NET_5,
            .transfer = net_5_device_mask,
            .target = net_5_device_mask
        };
        load_register(dev_objects, &data);
        save_dev_objects(dev_objects, values);
    }

    char *json_string = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
    mqtt_publish(mqtt_result_topic, json_string, strlen(json_string), 0, 0);
    free(json_string);

    set_status(MQTT_STATUS_IDLE);
    x3d_processing_task_handle = NULL;
    vTaskDelete(NULL);
}

void process_status(cJSON *cmd)
{
    if (x3d_processing_task_handle != NULL)
    {
        ESP_LOGE(TAG, "X3D message processing in progress");
        return;
    }
    ESP_LOGI(TAG, "process_status");
    xTaskCreate(process_status_task, "process_status_task", 4096, NULL, 10, &x3d_processing_task_handle);
}

/***********************************************
 * MQTT message handler
 */

static inline int mqtt_topic_match(char *topic)
{
    return strncmp(mqtt_topic_prefix, topic, MQTT_TOPIC_PREFIX_LEN);
}

void mqtt_data(char *topic, char *data)
{
    if (mqtt_topic_match(topic) != 0)
    {
        return;
    }

    char *sub_topic = &topic[MQTT_TOPIC_PREFIX_LEN];
    if (strcmp(sub_topic, MQTT_TOPIC_CMD) == 0)
    {
        cJSON *cmd = cJSON_Parse(data);
        cJSON *action = cJSON_GetObjectItem(cmd, MQTT_JSON_ACTION);
        if (action != NULL)
        {
            if (strcmp(action->valuestring, MQTT_CMD_PAIR) == 0)
            {
                process_pairing(cmd);
            }
            else if (strcmp(action->valuestring, MQTT_CMD_READ) == 0)
            {
                process_reading(cmd);
            }
            else if (strcmp(action->valuestring, MQTT_CMD_WRITE) == 0)
            {
                process_writing(cmd);
            }
            else if (strcmp(action->valuestring, MQTT_CMD_UNPAIR) == 0)
            {
                process_unpairing(cmd);
            }
            else if (strcmp(action->valuestring, MQTT_CMD_OUTDOOR_TEMP) == 0)
            {
                process_outdoor_temp(cmd);
            }
            else if (strcmp(action->valuestring, MQTT_CMD_INFO) == 0)
            {
                process_info(cmd);
            }
            else if (strcmp(action->valuestring, MQTT_CMD_STATUS) == 0)
            {
                process_status(cmd);
            }
        }
        cJSON_Delete(cmd);
    }
}

void mqtt_connected(void)
{
    char target_topic[80];
    strcpy (target_topic, mqtt_topic_prefix);
    strcat (target_topic, MQTT_TOPIC_CMD);
    mqtt_subscribe(target_topic, 0);
    set_status(MQTT_STATUS_IDLE);
    led_color(0, 255, 0);
}

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    led_init();

    nvs_handle_t nvs_ctx_handle;
    if (nvs_open("ctx", NVS_READONLY, &nvs_ctx_handle) == ESP_OK)
    {
        if (nvs_get_u16(nvs_ctx_handle, NVS_NET_4_DEVICE_MASK, &net_4_device_mask) != ESP_OK)
        {
            net_4_device_mask = 0;
        }
        if (nvs_get_u16(nvs_ctx_handle, NVS_NET_5_DEVICE_MASK, &net_5_device_mask) != ESP_OK)
        {
            net_5_device_mask = 0;
        }
        nvs_close(nvs_ctx_handle);
    }

    ESP_LOGI(TAG, "paired device mask Net 4: %04x Net 5: %04x", net_4_device_mask, net_5_device_mask);

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    // prepare device id
    x3d_set_device_id(mac[3] << 16 | mac[4] << 8 | mac[5]);
    mqtt_topic_prefix = calloc(MQTT_TOPIC_PREFIX_LEN + 1, sizeof(char));
    sprintf(mqtt_topic_prefix, "/device/esp32/%02x%02x%02x", mac[3], mac[4], mac[5]);

    // prepare status topic
    mqtt_status_topic = calloc(strlen(mqtt_topic_prefix) + strlen(MQTT_TOPIC_STATUS) + 1, sizeof(char));
    strcpy (mqtt_status_topic, mqtt_topic_prefix);
    strcat (mqtt_status_topic, MQTT_TOPIC_STATUS);

    // prepare result topic
    mqtt_result_topic = calloc(strlen(mqtt_topic_prefix) + strlen(MQTT_TOPIC_RESULT) + 1, sizeof(char));
    strcpy (mqtt_result_topic, mqtt_topic_prefix);
    strcat (mqtt_result_topic, MQTT_TOPIC_RESULT);

    ESP_ERROR_CHECK(rfm_init());
    ESP_ERROR_CHECK(wifi_init_sta());
    mqtt_app_start(mqtt_status_topic, "off", 3, 0, 1);
}
