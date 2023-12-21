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

#include "wifi.h"
#include "rfm.h"
#include "mqtt.h"
#include "led.h"
#include "ota.h"
#include "x3d_handler.h"

#define MQTT_TOPIC_PREFIX_LEN               20
#define MQTT_TOPIC_STATUS                   "/status"
#define MQTT_TOPIC_RESULT                   "/result"
#define MQTT_TOPIC_OUTDOOR_TEMP             "/outdoorTemp"
#define MQTT_TOPIC_DEVICE_STATUS            "/deviceStatus"
#define MQTT_TOPIC_RESET                    "/reset"
#define MQTT_TOPIC_PAIR                     "/pair"
#define MQTT_TOPIC_UNPAIR                   "/unpair"
#define MQTT_TOPIC_READ                     "/read"

#define MQTT_ACTION_READ                    "read"

#define MQTT_TOPIC_CMD                      "/cmd"
#define MQTT_CMD_WRITE                      "write"

#define MQTT_STATUS_IDLE                    "idle"
#define MQTT_STATUS_PAIRING                 "pairing"
#define MQTT_STATUS_PAIRING_SUCCESS         "pairing success"
#define MQTT_STATUS_PAIRING_FAILED          "pairing failed"
#define MQTT_STATUS_READING                 "reading"
#define MQTT_STATUS_WRITING                 "writing"
#define MQTT_STATUS_UNPAIRING               "unpairing"
#define MQTT_STATUS_TEMP                    "temp"
#define MQTT_STATUS_STATUS                  "status"
#define MQTT_STATUS_RESET                   "reset"

#define MQTT_JSON_ACTION                    "action"
#define MQTT_JSON_NETWORK                   "net"
#define MQTT_JSON_ACK                       "ack"
#define MQTT_JSON_REGISTER_HIGH             "regHigh"
#define MQTT_JSON_REGISTER_LOW              "regLow"
#define MQTT_JSON_VALUES                    "values"
#define MQTT_JSON_TARGET                    "target"

#define MQTT_JSON_ROOM_TEMP                 "roomTemp"
#define MQTT_JSON_SET_POINT                 "setPoint"
#define MQTT_JSON_ENABLED                   "enabled"
#define MQTT_JSON_ON_AIR                    "onAir"
#define MQTT_JSON_SET_POINT_DAY             "setPointDay"
#define MQTT_JSON_SET_POINT_NIGHT           "setPointNight"
#define MQTT_JSON_SET_POINT_DEFROST         "setPointDefrost"
#define MQTT_JSON_FLAGS                     "flags"

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

#define X3D_REG_START_PAIR                  0x1401
#define X3D_REG_ROOM_TEMP                   0x1511
#define X3D_REG_OUTDOOR_TEMP                0x1521
#define X3D_REG_SETPOINT_STATUS             0x1611
#define X3D_REG_ERROR_STATUS                0x1621
#define X3D_REG_ON_OFF                      0x1641
#define X3D_REG_MODE_TIME                   0x1661
#define X3D_REG_SETPOINT_DEFROST            0x1681
#define X3D_REG_SETPOINT_NIGHT_DAY          0x1691
#define X3D_REG_ON_TIME_LSB                 0x1910
#define X3D_REG_ON_TIME_MSB                 0x1990

#define X3D_REG_H(R)                        (((R) >> 8) & 0xff)
#define X3D_REG_L(R)                        ((R) & 0xff)

#define FLAG_TO_BITFIELD(F, M)              (((F) & (M)) == (M))

typedef struct {
    uint16_t room_temp;
    uint8_t set_point;
    uint8_t set_point_day;
    uint8_t set_point_night;
    uint8_t set_point_defrost;
    int on_air: 1;
    int enabled: 1;
    int defrost: 1;
    int timed: 1;
    int heater_on: 1;
    int heater_stopped: 1;
    int window_open: 1;
    int no_temp_sensor: 1;
    int battery_low: 1;
} x3d_device_t;

static const char *TAG = "MAIN";

static char *mqtt_topic_prefix = NULL;

TaskHandle_t x3d_processing_task_handle = NULL;

static uint16_t net_4_device_mask = 0;
static uint16_t net_5_device_mask = 0;

static x3d_device_t *net_4_devices[X3D_MAX_NET_DEVICES] = { NULL };
static x3d_device_t *net_5_devices[X3D_MAX_NET_DEVICES] = { NULL };

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

static inline x3d_device_t **get_devices_list(uint8_t network)
{
    switch (network)
    {
        case NET_4:
            return net_4_devices;
        case NET_5:
            return net_5_devices;
        default:
            return NULL;
    }
}

/**
 * @brief Publishes message to sub topic of device
 *
 * @param subtopic
 * @param data
 * @param len
 * @param qos
 * @param retain
 * @return int
 */
int mqtt_publish_subtopic(const char *subtopic, const char *data, int len, int qos, int retain)
{
    char topic[128];
    strcpy(topic, mqtt_topic_prefix);
    strcat(topic, subtopic);
    return mqtt_publish(topic, data, len, qos, retain);
}

/**
 * @brief Subscribe to sub topic of device
 *
 * @param subtopic
 * @param qos
 * @return int
 */
int mqtt_subscribe_subtopic(const char *subtopic, int qos)
{
    char topic[128];
    strcpy(topic, mqtt_topic_prefix);
    strcat(topic, subtopic);
    return mqtt_subscribe(topic, qos);
}

/**
 * @brief Set the status of the controller
 *
 * @param status
 */
void set_status(char *status)
{
    mqtt_publish_subtopic(MQTT_TOPIC_STATUS, status, strlen(status), 0, 1);
}

void publish_device(x3d_device_t *device, uint8_t network, int id, int update)
{
    char topic[128];
    sprintf(topic, "%s/%d/%d", mqtt_topic_prefix, network, id);
    if (device == NULL && update == 0)
    {
        mqtt_publish(topic, NULL, 0, 0, 1);
    }
    else if (device != NULL)
    {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, MQTT_JSON_ROOM_TEMP, (double)device->room_temp / 100.0);
        cJSON_AddNumberToObject(root, MQTT_JSON_SET_POINT, (double)device->set_point * 0.5);
        cJSON_AddNumberToObject(root, MQTT_JSON_SET_POINT_DAY, (double)device->set_point_day * 0.5);
        cJSON_AddNumberToObject(root, MQTT_JSON_SET_POINT_NIGHT, (double)device->set_point_night * 0.5);
        cJSON_AddNumberToObject(root, MQTT_JSON_SET_POINT_DEFROST, (double)device->set_point_defrost * 0.5);
        cJSON_AddBoolToObject(root, MQTT_JSON_ENABLED, device->enabled);
        cJSON_AddBoolToObject(root, MQTT_JSON_ON_AIR, device->on_air);
        cJSON *flags = cJSON_AddArrayToObject(root, MQTT_JSON_FLAGS);
        if (device->defrost)
        {
	        cJSON_AddItemToArray(flags, cJSON_CreateString(MQTT_JSON_DEFROST));
        }
        if (device->timed)
        {
	        cJSON_AddItemToArray(flags, cJSON_CreateString(MQTT_JSON_TIMED));
        }
        if (device->heater_on)
        {
	        cJSON_AddItemToArray(flags, cJSON_CreateString(MQTT_JSON_HEATER_ON));
        }
        if (device->heater_stopped)
        {
	        cJSON_AddItemToArray(flags, cJSON_CreateString(MQTT_JSON_HEATER_STOPPED));
        }
        if (device->window_open)
        {
	        cJSON_AddItemToArray(flags, cJSON_CreateString(MQTT_JSON_WINDOW_OPEN));
        }
        if (device->no_temp_sensor)
        {
	        cJSON_AddItemToArray(flags, cJSON_CreateString(MQTT_JSON_NO_TEMP_SENSOR));
        }
        if (device->battery_low)
        {
	        cJSON_AddItemToArray(flags, cJSON_CreateString(MQTT_JSON_BATTERY_LOW));
        }
        char *json_string = cJSON_PrintUnformatted(root);
        cJSON_Delete(root);
        mqtt_publish(topic, json_string, strlen(json_string), 0, 1);
        free(json_string);
    }
}

void init_mqtt_topic_devices(void)
{
    for (int i = 0; i < X3D_MAX_NET_DEVICES; i++)
    {
        publish_device(net_4_devices[i], NET_4, i, 0);
        publish_device(net_5_devices[i], NET_5, i, 0);
    }
}

/**
 * @brief Updates the current network mask in the NVS and publishes the current masks
 *
 * @param network
 * @param mask
 */
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

static void __attribute__((noreturn)) end_task(void *arg)
{
    if (arg != NULL)
    {
        free(arg);
    }
    set_status(MQTT_STATUS_IDLE);
    x3d_processing_task_handle = NULL;
    vTaskDelete(NULL);
    while (1); // should not be reached
}

void pairing_task(void *arg)
{
    uint8_t network  = strtoul(arg, NULL, 10);
    if (!valid_network(network))
    {
        end_task(arg);
    }

    x3d_pairing_data_t data = {
        .network = network,
        .transfer = get_network_mask(network)
    };

    if (no_of_devices(data.transfer) >= X3D_MAX_NET_DEVICES)
    {
        end_task(arg);
    }

    set_status(MQTT_STATUS_PAIRING);
    int target_device_no = x3d_pairing_proc(&data);
    if (target_device_no == -1)
    {
        set_status(MQTT_STATUS_PAIRING_FAILED);
    }
    else
    {
        set_status(MQTT_STATUS_PAIRING_SUCCESS);
        update_nvs_device_mask(data.network, data.transfer);

        x3d_device_t **devices = get_devices_list(data.network);
        if (devices[target_device_no] == NULL)
        {
            devices[target_device_no] = (x3d_device_t *)calloc(1, sizeof(x3d_device_t));
            devices[target_device_no]->on_air = 1;
            publish_device(devices[target_device_no], data.network, target_device_no, 0);
        }
    }

    end_task(arg);
}

void unpairing_task(void *arg)
{
    char * pEnd;
    uint8_t network = strtoul(arg, &pEnd, 10);
    uint8_t target = strtoul(pEnd, NULL, 10);

    if (!valid_network(network))
    {
        end_task(arg);
    }

    x3d_unpairing_data_t data = {
        .network = network,
        .target = target & 0x0f,
        .transfer = get_network_mask(network)
    };

    if (data.transfer == 0 || (data.transfer & data.target) == 0)
    {
        end_task(arg);
    }

    set_status(MQTT_STATUS_UNPAIRING);
    x3d_unpairing_proc(&data);
    update_nvs_device_mask(data.network, data.transfer);

    x3d_device_t **devices = get_devices_list(data.network);
    if (devices[data.target] != NULL)
    {
        free(devices[data.target]);
        devices[data.target] = NULL;
        publish_device(NULL, data.network, data.target, 0);
    }

    end_task(arg);
}

void reading_task(void *arg)
{
    char *pEnd;
    uint8_t network = strtoul(arg, &pEnd, 10);
    uint16_t target = strtoul(pEnd, &pEnd, 10);
    uint8_t register_high = strtoul(pEnd, &pEnd, 10);
    char *pCheckEnd = pEnd;
    uint8_t register_low = strtoul(pEnd, &pCheckEnd, 10);

    // check if only three parameters given
    if (pCheckEnd == pEnd)
    {
        register_low = register_high;
        register_high = target;
        target = get_network_mask(network);
    }

    if (!valid_network(network) || no_of_devices(target) == 0)
    {
        end_task(arg);
    }

    x3d_read_data_t data = {
        .network = network,
        .transfer = get_network_mask(network),
        .target = target,
        .register_high = register_high,
        .register_low = register_low
    };

    if (data.transfer == 0 || (data.transfer & data.target) == 0)
    {
        end_task(arg);
    }

    set_status(MQTT_STATUS_READING);
    x3d_standard_msg_payload_t *payload = x3d_reading_proc(&data);

    ESP_LOGI(TAG, "read register %02x - %02x from %04x", payload->reg_high, payload->reg_low, payload->target_ack);

    int data_slots = ((payload->action & 0xf0) >> 4) + 1;
    cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, MQTT_JSON_ACTION, MQTT_ACTION_READ);
	cJSON_AddNumberToObject(root, MQTT_JSON_NETWORK, data.network);
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

    mqtt_publish_subtopic(MQTT_TOPIC_RESULT, json_string, strlen(json_string), 0, 0);
    free(json_string);

    end_task(arg);
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

    mqtt_publish_subtopic(MQTT_TOPIC_RESULT, json_string, strlen(json_string), 0, 0);
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
    double temp = strtod(arg, NULL);
    set_status(MQTT_STATUS_TEMP);
    x3d_temp_data_t data = {
        .outdoor = 0x01,
        .temp = temp * 100.0
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

    end_task(arg);
}

void read_reg_to_devices(x3d_device_t **devices, x3d_read_data_t *data, uint16_t reg)
{
    data->register_high = X3D_REG_H(reg);
    data->register_low = X3D_REG_L(reg);
    x3d_standard_msg_payload_t *payload = x3d_reading_proc(data);
    for (int i = 0; i < X3D_MAX_NET_DEVICES; i++)
    {
        if (devices[i] == NULL)
        {
            continue;
        }
        if (payload->target_ack & (1 << i))
        {
            devices[i]->on_air = 1;
            switch (reg)
            {
                case X3D_REG_ROOM_TEMP:
                    devices[i]->room_temp = payload->data[i];
                    break;
                case X3D_REG_SETPOINT_STATUS:
                    devices[i]->set_point = payload->data[i] & 0xff;
                    uint16_t flags = payload->data[i] & 0xff00;
                    devices[i]->defrost = FLAG_TO_BITFIELD(flags, X3D_FLAG_DEFROST);
                    devices[i]->timed = FLAG_TO_BITFIELD(flags, X3D_FLAG_TIMED);
                    devices[i]->heater_on = FLAG_TO_BITFIELD(flags, X3D_FLAG_HEATER_ON);
                    devices[i]->heater_stopped = FLAG_TO_BITFIELD(flags, X3D_FLAG_HEATER_STOPPED);
                    break;
                case X3D_REG_ERROR_STATUS:
                    devices[i]->window_open = FLAG_TO_BITFIELD(payload->data[i], X3D_FLAG_WINDOW_OPEN);
                    devices[i]->no_temp_sensor = FLAG_TO_BITFIELD(payload->data[i], X3D_FLAG_NO_TEMP_SENSOR);
                    devices[i]->battery_low = FLAG_TO_BITFIELD(payload->data[i], X3D_FLAG_BATTERY_LOW);
                    break;
                case X3D_REG_ON_OFF:
                    devices[i]->enabled = FLAG_TO_BITFIELD(payload->data[i], 0x0001);
                    break;
                case X3D_REG_MODE_TIME:
                    break;
                case X3D_REG_SETPOINT_DEFROST:
                    devices[i]->set_point_defrost = payload->data[i] & 0xff;
                    break;
                case X3D_REG_SETPOINT_NIGHT_DAY:
                    devices[i]->set_point_night = payload->data[i] & 0xff;
                    devices[i]->set_point_day = (payload->data[i] >> 8) & 0xff;
                    break;
            }
        }
        else if (payload->target & (1 << i))
        {
            devices[i]->on_air = 0;
        }
    }
}

void load_register(x3d_device_t **devices, uint8_t network, uint16_t device_mask)
{
    x3d_read_data_t data = {
        .network = network,
        .transfer = device_mask,
        .target = device_mask
    };

    read_reg_to_devices(devices, &data, X3D_REG_ROOM_TEMP);
    read_reg_to_devices(devices, &data, X3D_REG_SETPOINT_STATUS);
    read_reg_to_devices(devices, &data, X3D_REG_ERROR_STATUS);
    read_reg_to_devices(devices, &data, X3D_REG_ON_OFF);
    read_reg_to_devices(devices, &data, X3D_REG_SETPOINT_DEFROST);
    read_reg_to_devices(devices, &data, X3D_REG_SETPOINT_NIGHT_DAY);
}

void device_status_task(void *arg)
{
    uint8_t network = strtoul(arg, NULL, 10);
    if (!valid_network(network))
    {
        end_task(arg);
    }

    set_status(MQTT_STATUS_STATUS);
    uint16_t device_mask = get_network_mask(network);
    if (no_of_devices(device_mask))
    {
        x3d_device_t **devices = get_devices_list(network);
        load_register(devices, network, device_mask);
        for (int i = 0; i < X3D_MAX_NET_DEVICES; i++)
        {
            publish_device(devices[i], network, i, 1);
        }
    }

    end_task(arg);
}

void execute_task(TaskFunction_t pxTaskCode, const char * const pcName, const configSTACK_DEPTH_TYPE usStackDepth, void * const pvParameters)
{
    if (x3d_processing_task_handle != NULL)
    {
        ESP_LOGE(TAG, "X3D message processing in progress");
        return;
    }
    ESP_LOGI(TAG, "Start: %s", pcName);
    xTaskCreate(pxTaskCode, pcName, usStackDepth, pvParameters, 10, &x3d_processing_task_handle);
}

/***********************************************
 * MQTT message handler
 */

void mqtt_data(char *topic, char *data)
{
    if (strncmp(mqtt_topic_prefix, topic, MQTT_TOPIC_PREFIX_LEN) != 0)
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
            if (strcmp(action->valuestring, MQTT_CMD_WRITE) == 0)
            {
                process_writing(cmd);
            }
        }
        cJSON_Delete(cmd);
    }
    else if (strcmp(sub_topic, MQTT_TOPIC_PAIR) == 0)
    {
        execute_task(pairing_task, "pairing_task", 4096, strdup(data));
    }
    else if (strcmp(sub_topic, MQTT_TOPIC_UNPAIR) == 0)
    {
        execute_task(unpairing_task, "unpairing_task", 2048, strdup(data));
    }
    else if (strcmp(sub_topic, MQTT_TOPIC_READ) == 0)
    {
        execute_task(reading_task, "reading_task", 4096, strdup(data));
    }
    else if (strcmp(sub_topic, MQTT_TOPIC_OUTDOOR_TEMP) == 0)
    {
        execute_task(outdoor_temp_task, "outdoor_temp_task", 2048, strdup(data));
    }
    else if (strcmp(sub_topic, MQTT_TOPIC_DEVICE_STATUS) == 0)
    {
        execute_task(device_status_task, "device_status_task", 4096, strdup(data));
    }
    else if (strcmp(sub_topic, MQTT_TOPIC_RESET) == 0)
    {
        set_status(MQTT_STATUS_RESET);
        ESP_LOGI(TAG, "Prepare to restart system!");
        esp_restart();
    }
}

void mqtt_connected(void)
{
    mqtt_subscribe_subtopic(MQTT_TOPIC_CMD, 0);
    mqtt_subscribe_subtopic(MQTT_TOPIC_OUTDOOR_TEMP, 0);
    mqtt_subscribe_subtopic(MQTT_TOPIC_DEVICE_STATUS, 0);
    mqtt_subscribe_subtopic(MQTT_TOPIC_RESET, 0);
    mqtt_subscribe_subtopic(MQTT_TOPIC_PAIR, 0);
    mqtt_subscribe_subtopic(MQTT_TOPIC_UNPAIR, 0);
    set_status(MQTT_STATUS_IDLE);
    init_mqtt_topic_devices();
    led_color(0, 20, 0);
}

void app_main(void)
{
    ota_precheck();
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    led_init();

    ESP_ERROR_CHECK(wifi_init_sta());
    ota_execute();

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

    // init device cache
    for (int i = 0; i < X3D_MAX_NET_DEVICES; i++)
    {
        if (net_4_device_mask & (1 << i))
        {
            net_4_devices[i] = (x3d_device_t *)calloc(1, sizeof(x3d_device_t));
        }
        if (net_5_device_mask & (1 << i))
        {
            net_5_devices[i] = (x3d_device_t *)calloc(1, sizeof(x3d_device_t));
        }
    }

    ESP_ERROR_CHECK(rfm_init());

    char topic[128];
    strcpy(topic, mqtt_topic_prefix);
    strcpy(topic, MQTT_TOPIC_STATUS);
    mqtt_app_start(topic, "off", 3, 0, 1);
}
