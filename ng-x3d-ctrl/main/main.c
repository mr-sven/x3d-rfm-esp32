/**
 * @file main.c
 * @author Sven Fabricius (sven.fabricius@livediesel.de)
 * @brief Next Gen X3D Control Gateway
 * @version 0.1
 * @date 2024-02-13
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <string.h>
#include <stdarg.h>

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
#include "x3d_device.h"

#define NET_4                          4
#define NET_5                          5

#define X3D_REG_ON_OFF_ON              0x0739
#define X3D_REG_ON_OFF_OFF             0x0738

// task execution args
typedef struct {
    uint8_t network;
    uint16_t target_mask;
    char *args;
} task_arg_t;

// type of X3D message
typedef enum {
    ENABLE_UNKNOWN = -1,
    ENABLE_DAY = 0,
    ENABLE_NIGHT = 1,
    ENABLE_DEFROST = 2,
    ENABLE_CUSTOM = 3,
    ENABLE_TIMED = 4,
} enable_mode_t;

static const char JSON_ACTION[] =                    "action";
static const char JSON_NETWORK[] =                   "net";
static const char JSON_ACK[] =                       "ack";
static const char JSON_REGISTER_HIGH[] =             "regHigh";
static const char JSON_REGISTER_LOW[] =              "regLow";
static const char JSON_VALUES[] =                    "values";

// using string constants instead of defines to save flash memory
static const char NVS_NET_4_DEVICES[] =              "net_4_devices";
static const char NVS_NET_5_DEVICES[] =              "net_5_devices";

static const char MQTT_TOPIC_CMD[] =                 "/cmd";
static const char MQTT_TOPIC_RESULT[] =              "/result";

static const char COMMAND_RESET[] =                  "reset";
static const char COMMAND_OUTDOOR_TEMP[] =           "outdoor-temp "; // include space because of command arguments
static const char COMMAND_PAIR[] =                   "pair";
static const char COMMAND_PAIR_NET[] =               "pair ";
static const char COMMAND_DEVICE_STATUS[] =          "device-status";
static const char COMMAND_DEVICE_STATUS_SHORT[] =    "device-status-short";
static const char COMMAND_READ[] =                   "read "; // include space because of command arguments
static const char COMMAND_ENABLE[] =                 "enable "; // include space because of command arguments
static const char COMMAND_DISABLE[] =                "disable";
static const char COMMAND_WRITE[] =                  "write "; // include space because of command arguments
static const char COMMAND_UNPAIR[] =                 "unpair";

static const char MQTT_STATUS_OFF[] =                "off";
static const char MQTT_STATUS_IDLE[] =               "idle";
static const char MQTT_STATUS_READING[] =            "reading";
static const char MQTT_STATUS_PAIRING[] =            "pairing";
static const char MQTT_STATUS_PAIRING_SUCCESS[] =    "pairing success";
static const char MQTT_STATUS_PAIRING_FAILED[] =     "pairing failed";
/*
static const char MQTT_STATUS_WRITING[] =            "writing"; */
static const char MQTT_STATUS_UNPAIRING[] =          "unpairing";
static const char MQTT_STATUS_STATUS[] =             "status";
static const char MQTT_STATUS_TEMP[] =               "temp";
static const char MQTT_STATUS_RESET[] =              "reset";
static const char MQTT_STATUS_START[] =              "start";

static const char TAG[] = "MAIN";

#define MQTT_TOPIC_PREFIX_LEN   17
#define MQTT_TOPIC_PREFIX_SIZE  (MQTT_TOPIC_PREFIX_LEN + 1)

#define MQTT_TOPIC_STATUS_LEN   24
#define MQTT_TOPIC_STATUS_SIZE  (MQTT_TOPIC_STATUS_LEN + 1)

// Prefix used for mqtt topics
static char mqtt_topic_prefix[MQTT_TOPIC_PREFIX_SIZE]; // strlen("device/x3d/aabbcc") = 17 + 1

// status topic
static char mqtt_topic_status[MQTT_TOPIC_STATUS_SIZE]; // strlen("device/x3d/aabbcc/status") = 24 + 1

// list of devices
static x3d_device_t net_4_devices[X3D_MAX_NET_DEVICES] = {0};
static x3d_device_t net_5_devices[X3D_MAX_NET_DEVICES] = {0};

// transfer masks
static uint16_t net_4_transfer_mask = 0;
static uint16_t net_5_transfer_mask = 0;

TaskHandle_t processing_task_handle = NULL;

enable_mode_t str_to_enabe_mode(const char *str)
{
    if (strcmp(str, "day") == 0) { return ENABLE_DAY; }
    else if (strcmp(str, "night") == 0) { return ENABLE_NIGHT; }
    else if (strcmp(str, "defrost") == 0) { return ENABLE_DEFROST; }
    else if (strcmp(str, "custom") == 0) { return ENABLE_CUSTOM; }
    else if (strcmp(str, "timed") == 0) { return ENABLE_TIMED; }
    return ENABLE_UNKNOWN;
}

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
        return net_4_transfer_mask;
    case NET_5:
        return net_5_transfer_mask;
    default:
        return 0;
    }
}

static inline x3d_device_t *get_devices_list(uint8_t network)
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
 * @brief Initialize device data list
 *
 * @param nvs_devices
 * @param devices
 * @return uint16_t device transfer mask
 */
uint16_t init_device_data(x3d_device_type_t *nvs_devices, x3d_device_t *devices)
{
    uint16_t mask = 0;
    for (int i = 0; i < X3D_MAX_NET_DEVICES; i++)
    {
        if (x3d_device_from_type(&devices[i], nvs_devices[i]))
        {
            mask |= (1 << i);
        }
    }
    return mask;
}

/**
 * @brief Get the target device mask by feature
 *
 * @param devices
 * @param feature
 * @return uint16_t
 */
uint16_t get_target_mask_by_feature(x3d_device_t *devices, x3d_device_feature_t feature)
{
    uint16_t mask = 0;
    for (int i = 0; i < X3D_MAX_NET_DEVICES; i++)
    {
        if (x3d_device_feature_list[devices[i].type] & feature)
        {
            mask |= (1 << i);
        }
    }
    return mask;
}

/**
 * @brief Publishes a device to its topic
 *
 * @param device
 * @param network
 * @param id
 * @param force
 */
void publish_device(x3d_device_t *device, uint8_t network, uint8_t id, bool force)
{
    char topic[64];
    snprintf(topic, 64, "%s/net-%d/dest/%d/status", mqtt_topic_prefix, network, id);
    cJSON *root = NULL;

    switch (device->type)
    {
        case X3D_DEVICE_TYPE_RF66XX:
            root = x3d_rf66xx_to_json((x3d_rf66xx_t *)device->data);
            break;
        default:
            if (force)
            {
                mqtt_publish(topic, NULL, 0, 0, 1);
            }
            return;
    }

    ESP_LOGI(TAG, "topic: %s", topic);
    if (root != NULL)
    {
        char *json_string = cJSON_PrintUnformatted(root);
        cJSON_Delete(root);
        mqtt_publish(topic, json_string, strlen(json_string), 0, 1);
        free(json_string);
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
    snprintf(topic, 128, "%s%s", mqtt_topic_prefix, subtopic);
    return mqtt_publish(topic, data, len, qos, retain);
}

/**
 * @brief Set the status of the controller
 *
 * @param status
 */
void set_status(const char *status)
{
    mqtt_publish(mqtt_topic_status, status, strlen(status), 0, 1);
}

/**
 * @brief Subscribe to sub topic of device
 *
 * @param subtopic
 * @return int
 */
int mqtt_subscribe_subtopic(const char *subtopic)
{
    char topic[128];
    snprintf(topic, 128, "%s%s", mqtt_topic_prefix, subtopic);
    return mqtt_subscribe(topic, 0);
}

/**
 * @brief Subscribe to subtopic of network
 *
 * @param subtopic
 * @param net
 * @return int
 */
int mqtt_subscribe_net_subtopic(const char *subtopic, int net)
{
    char topic[128];
    snprintf(topic, 128, "%s/net-%d%s", mqtt_topic_prefix, net, subtopic);
    return mqtt_subscribe(topic, 0);
}

/**
 * @brief Subscribe to subtopic of network
 *
 * @param subtopic
 * @param net
 * @return int
 */
int mqtt_subscribe_net_device_subtopic(const char *subtopic, int net)
{
    char topic[128];
    snprintf(topic, 128, "%s/net-%d/dest/+%s", mqtt_topic_prefix, net, subtopic);
    return mqtt_subscribe(topic, 0);
}

/**
 * @brief saves the devices type list to nvs
 *
 * @param network target network
 * @param devices list of devices
 */
void save_devices_to_nvs(uint8_t network, x3d_device_t *devices)
{
    const char *key;
    switch (network)
    {
    case NET_4:
        key = NVS_NET_4_DEVICES;
        break;
    case NET_5:
        key = NVS_NET_5_DEVICES;
        break;
    default:
        return;
    }

    // build storage blob
    x3d_device_type_t nvs_devices[X3D_MAX_NET_DEVICES] = { X3D_DEVICE_TYPE_NONE };
    for (int i = 0; i < X3D_MAX_NET_DEVICES; i++)
    {
        nvs_devices[i] = devices[i].type;
    }

    nvs_handle_t nvs_ctx_handle;
    if (nvs_open("ctx", NVS_READWRITE, &nvs_ctx_handle) == ESP_OK)
    {
        nvs_set_blob(nvs_ctx_handle, key, nvs_devices, X3D_MAX_NET_DEVICES);
        nvs_commit(nvs_ctx_handle);
        nvs_close(nvs_ctx_handle);
    }
}

void remove_device_data(uint8_t network, uint16_t remove_mask)
{
    x3d_device_t *devices = NULL;

    // get devices list and remove from transfer mask
    switch (network)
    {
    case NET_4:
        devices = net_4_devices;
        net_4_transfer_mask &= ~(remove_mask);
        break;
    case NET_5:
        devices = net_5_devices;
        net_5_transfer_mask &= ~(remove_mask);
        break;
    default:
        return;
    }

    // remove device data
    for (int i = 0; i < X3D_MAX_NET_DEVICES; i++)
    {
        if (remove_mask & (1 << i) && devices[i].data != NULL)
        {
            free(devices[i].data);
            devices[i].type = X3D_DEVICE_TYPE_NONE;
            publish_device(NULL, network, i, true);
        }
    }

    save_devices_to_nvs(network, devices);
}

void create_device_data(uint8_t network, x3d_device_type_t type, uint8_t target_no)
{
    if (target_no >= X3D_MAX_NET_DEVICES || type == X3D_DEVICE_TYPE_NONE)
    {
        return;
    }

    x3d_device_t *devices = NULL;

    // get devices list and remove from transfer mask
    switch (network)
    {
    case NET_4:
        devices = net_4_devices;
        net_4_transfer_mask |= (1 << target_no);
        break;
    case NET_5:
        devices = net_5_devices;
        net_5_transfer_mask |= (1 << target_no);
        break;
    default:
        return;
    }

    x3d_device_from_type(&devices[target_no], type);
    save_devices_to_nvs(network, devices);
    publish_device(&devices[target_no], network, target_no, true);
}

void read_reg_to_devices(x3d_device_t *devices, x3d_read_data_t *data, uint16_t reg)
{
    data->register_high                 = X3D_REG_H(reg);
    data->register_low                  = X3D_REG_L(reg);
    x3d_standard_msg_payload_t *payload = x3d_reading_proc(data);
    for (int i = 0; i < X3D_MAX_NET_DEVICES; i++)
    {
        int req = payload->target & (1 << i);
        int ack = payload->target_ack & (1 << i);

        switch (devices[i].type)
        {
            case X3D_DEVICE_TYPE_RF66XX:
                x3d_rf66xx_set_from_reg((x3d_rf66xx_t *)devices[i].data, req, ack, reg, payload->data[i]);
                break;
            default:
                break;
        }
    }
}

void set_reg_same(x3d_write_data_t * data, uint16_t reg, uint16_t value)
{
    data->register_high = X3D_REG_H(reg);
    data->register_low  = X3D_REG_L(reg);
    for (int i = 0; i < X3D_MAX_PAYLOAD_DATA_FIELDS; i++)
    {
        if (data->target & (1 << i))
        {
            data->values[i] = value;
        }
        else
        {
            data->values[i] = 0;
        }
    }
}

/*********************************************
 * Task Region
 */

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
    set_status(MQTT_STATUS_IDLE);
    processing_task_handle = NULL;
    vTaskDelete(NULL);
    while (1); // should not be reached
}

void outdoor_temp_task(void *arg)
{
    double temp = strtod(arg, NULL);
    set_status(MQTT_STATUS_TEMP);
    x3d_temp_data_t data = {
            .outdoor = X3D_HEADER_EXT_TEMP_OUTDOOR,
            .temp    = temp * 100.0,
    };

    data.network  = NET_4;
    data.transfer = net_4_transfer_mask;
    data.target = get_target_mask_by_feature(net_4_devices, X3D_DEVICE_FEATURE_OUTDOOR_TEMP);
    if (data.target != 0)
    {
        x3d_temp_proc(&data);
    }

    data.network  = NET_5;
    data.transfer = net_5_transfer_mask;
    data.target = get_target_mask_by_feature(net_5_devices, X3D_DEVICE_FEATURE_OUTDOOR_TEMP);
    if (data.target != 0)
    {
        x3d_temp_proc(&data);
    }

    end_task(arg);
}

void device_status_task(void *arg)
{
    uint8_t network = (uintptr_t)arg;
    if (!valid_network(network))
    {
        end_task(NULL); // arg is not a pointer
    }

    set_status(MQTT_STATUS_STATUS);
    uint16_t device_mask = get_network_mask(network);
    x3d_read_data_t data = {
            .network  = network,
            .transfer = device_mask,
            .target   = device_mask,
    };

    if (no_of_devices(device_mask))
    {
        x3d_device_t *devices = get_devices_list(network);
        read_reg_to_devices(devices, &data, X3D_REG_ROOM_TEMP);
        read_reg_to_devices(devices, &data, X3D_REG_SETPOINT_STATUS);
        read_reg_to_devices(devices, &data, X3D_REG_ERROR_STATUS);
        read_reg_to_devices(devices, &data, X3D_REG_ON_OFF);
        read_reg_to_devices(devices, &data, X3D_REG_SETPOINT_DEFROST);
        read_reg_to_devices(devices, &data, X3D_REG_SETPOINT_NIGHT_DAY);
        read_reg_to_devices(devices, &data, X3D_REG_ATT_POWER);

        for (int i = 0; i < X3D_MAX_NET_DEVICES; i++)
        {
            publish_device(&devices[i], network, i, false);
        }
    }

    end_task(NULL); // arg is not a pointer
}

void device_status_short_task(void *arg)
{
    uint8_t network = (uintptr_t)arg;
    if (!valid_network(network))
    {
        end_task(NULL); // arg is not a pointer
    }

    set_status(MQTT_STATUS_STATUS);
    uint16_t device_mask = get_network_mask(network);
    x3d_read_data_t data = {
            .network  = network,
            .transfer = device_mask,
            .target   = device_mask,
    };

    if (no_of_devices(device_mask))
    {
        x3d_device_t *devices = get_devices_list(network);
        read_reg_to_devices(devices, &data, X3D_REG_ROOM_TEMP);
        read_reg_to_devices(devices, &data, X3D_REG_SETPOINT_STATUS);
        read_reg_to_devices(devices, &data, X3D_REG_ERROR_STATUS);
        read_reg_to_devices(devices, &data, X3D_REG_ON_OFF);

        for (int i = 0; i < X3D_MAX_NET_DEVICES; i++)
        {
            publish_device(&devices[i], network, i, false);
        }
    }

    end_task(NULL); // arg is not a pointer
}

void network_pairing_task(void *arg)
{
    task_arg_t *args = arg;
    if (!valid_network(args->network))
    {
        free(args->args);
        end_task(args);
    }

    x3d_device_type_t type = x3d_device_type_from_string(args->args);
    if (type == X3D_DEVICE_TYPE_NONE)
    {
        free(args->args);
        end_task(args);
    }

    x3d_pairing_data_t data = {
            .network  = args->network,
            .transfer = get_network_mask(args->network),
    };

    if (no_of_devices(data.transfer) >= X3D_MAX_NET_DEVICES)
    {
        free(args->args);
        end_task(args);
    }

    // OPTIONAL: depending on device type could be diffrent pairing proc

    set_status(MQTT_STATUS_PAIRING);
    int target_device_no = x3d_pairing_proc(&data);
    if (target_device_no == -1)
    {
        set_status(MQTT_STATUS_PAIRING_FAILED);
    }
    else
    {
        set_status(MQTT_STATUS_PAIRING_SUCCESS);
        create_device_data(data.network, type, target_device_no);
    }

    free(args->args);
    end_task(args);
}

void reading_task(void *arg)
{
    task_arg_t *args = arg;

    char *pArg = NULL, *pEnd = NULL;
    uint8_t register_high = strtoul(args->args, &pArg, 10);
    uint8_t register_low  = strtoul(pArg, &pEnd, 10);

    x3d_read_data_t data = {
            .network       = args->network,
            .transfer      = get_network_mask(args->network),
            .target        = args->target_mask,
            .register_high = register_high,
            .register_low  = register_low,
    };

    if (data.transfer == 0 || (data.transfer & data.target) == 0)
    {
        free(args->args);
        end_task(arg);
    }

    set_status(MQTT_STATUS_READING);
    x3d_standard_msg_payload_t *payload = x3d_reading_proc(&data);

    ESP_LOGI(TAG, "read register %02x - %02x from %04x", payload->reg_high, payload->reg_low, payload->target_ack);

    int data_slots = ((payload->action & 0xf0) >> 4) + 1;
    cJSON *root    = cJSON_CreateObject();
    cJSON_AddStringToObject(root, JSON_ACTION, "read");
    cJSON_AddNumberToObject(root, JSON_NETWORK, data.network);
    cJSON_AddNumberToObject(root, JSON_ACK, payload->target_ack);
    cJSON_AddNumberToObject(root, JSON_REGISTER_HIGH, payload->reg_high);
    cJSON_AddNumberToObject(root, JSON_REGISTER_LOW, payload->reg_low);
    cJSON *values = cJSON_AddArrayToObject(root, JSON_VALUES);
    for (int i = 0; i < data_slots; i++)
    {
        cJSON_AddItemToArray(values, cJSON_CreateNumber(payload->data[i]));
    }
    char *json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    mqtt_publish_subtopic(MQTT_TOPIC_RESULT, json_string, strlen(json_string), 0, 0);
    free(json_string);

    free(args->args);
    end_task(arg);
}

void device_disable_task(void *arg)
{
    task_arg_t *args = arg;
    x3d_write_data_t data = {
            .network  = args->network,
            .transfer = get_network_mask(args->network),
            .target   = args->target_mask,
            .values   = {0},
    };

    if (data.transfer == 0 || (data.transfer & data.target) == 0)
    {
        end_task(arg);
    }

    // switch device off
    set_reg_same(&data, X3D_REG_ON_OFF, X3D_REG_ON_OFF_OFF);
    x3d_writing_proc(&data);

    // set time 0
    set_reg_same(&data, X3D_REG_MODE_TIME, 0);
    x3d_writing_proc(&data);

    // set time 0
    set_reg_same(&data, X3D_REG_SET_MODE_TEMP, 0);
    x3d_writing_proc(&data);

    end_task(arg);
}

void device_enable_task(void *arg)
{
    task_arg_t *args = arg;

    char *pArg = NULL;
    double temp;
    uint16_t outValue;
    uint16_t time = 0;
    enable_mode_t mode = str_to_enabe_mode(strtok_r(args->args, " ", &pArg));
    x3d_device_t *devices = get_devices_list(args->network);
    x3d_write_data_t data = {
            .network  = args->network,
            .transfer = get_network_mask(args->network),
            .target   = get_target_mask_by_feature(devices, X3D_DEVICE_FEATURE_TEMP_ACTOR) & args->target_mask,
            .register_high = X3D_REG_H(X3D_REG_SET_MODE_TEMP),
            .register_low  = X3D_REG_L(X3D_REG_SET_MODE_TEMP),
            .values   = {0},
    };

    if (data.transfer == 0 || (data.transfer & data.target) == 0)
    {
        free(args->args);
        end_task(arg);
    }

    // prepare mode and setpoint register
    switch (mode)
    {
        case ENABLE_DAY:
        case ENABLE_NIGHT:
        case ENABLE_DEFROST:
            for (int i = 0; i < X3D_MAX_PAYLOAD_DATA_FIELDS; i++)
            {
                if (data.target & (1 << i))
                {
                    uint8_t set_point_day = 0;
                    uint8_t set_point_night = 0;
                    uint8_t set_point_defrost = 0;

                    switch (devices[i].type)
                    {
                        case X3D_DEVICE_TYPE_RF66XX:
                            set_point_day = ((x3d_rf66xx_t *)devices[i].data)->set_point_day;
                            set_point_night = ((x3d_rf66xx_t *)devices[i].data)->set_point_night;
                            set_point_defrost = ((x3d_rf66xx_t *)devices[i].data)->set_point_defrost;
                            break;
                        default:
                            break;
                    }

                    switch (mode)
                    {
                        case ENABLE_DAY:
                            data.values[i] = set_point_day;
                            break;
                        case ENABLE_NIGHT:
                            data.values[i] = set_point_night;
                            break;
                        case ENABLE_DEFROST:
                            data.values[i] = set_point_defrost | X3D_FLAG_DEFROST;
                            break;
                        default: // is not reached
                            break;
                    }
                }
            }
            break;

        case ENABLE_CUSTOM:
            temp = strtod(pArg, &pArg);
            if (temp == 0)
            {
                end_task(arg);
            }

            // set mode and temperature
            outValue = (uint16_t)(temp * 2.0);
            set_reg_same(&data, X3D_REG_SET_MODE_TEMP, outValue);
            break;

        case ENABLE_TIMED:
            temp = strtod(pArg, &pArg);
            time = strtoul(pArg, &pArg, 10);
            if (temp == 0 || time == 0)
            {
                end_task(arg);
            }

            // set mode and temperature
            outValue = (uint16_t)(temp * 2.0) | X3D_FLAG_TIMED;
            set_reg_same(&data, X3D_REG_SET_MODE_TEMP, outValue);
            break;

        case ENABLE_UNKNOWN:
        default:
            end_task(arg);
            break;
    }
    // write setpoint and mode
    x3d_writing_proc(&data);

    // set time for timed mode or 0 for other modes
    set_reg_same(&data, X3D_REG_MODE_TIME, time);
    x3d_writing_proc(&data);

    // switch device on
    set_reg_same(&data, X3D_REG_ON_OFF, X3D_REG_ON_OFF_ON);
    x3d_writing_proc(&data);

    end_task(arg);
}

void unpairing_task(void *arg)
{
    task_arg_t *args = arg;

    x3d_unpairing_data_t data = {
            .network  = args->network,
            .target   = args->target_mask,
            .transfer = get_network_mask(args->network),
    };

    if (data.transfer == 0 || (data.transfer & data.target) == 0)
    {
        end_task(arg);
    }

    set_status(MQTT_STATUS_UNPAIRING);
    x3d_unpairing_proc(&data);

    remove_device_data(data.network, data.target);

    end_task(arg);
}

void device_pairing_task(void *arg)
{
    task_arg_t *args = arg;

    x3d_write_data_t data = {
            .network       = args->network,
            .transfer      = get_network_mask(args->network),
            .target        = args->target_mask,
            .register_high = X3D_REG_H(X3D_REG_START_PAIR),
            .register_low  = X3D_REG_L(X3D_REG_START_PAIR),
            .values        = {0},
    };

    if (data.transfer == 0 || (data.transfer & data.target) == 0)
    {
        end_task(arg);
    }

    set_status(MQTT_STATUS_PAIRING);
    x3d_writing_proc(&data);

    end_task(arg);
}



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

/*********************************************
 * MQTT Handler Region
 */

/**
 * @brief Executes device based commands
 *
 * @param data
 */
void handle_device_command(char *data)
{
    if (strcmp(data, COMMAND_RESET) == 0)
    {
        set_status(MQTT_STATUS_RESET);
        ESP_LOGI(TAG, "Prepare to restart system!");
        esp_restart();
    }
    else if (strncmp(data, COMMAND_OUTDOOR_TEMP, strlen(COMMAND_OUTDOOR_TEMP)) == 0)
    {
        execute_task(outdoor_temp_task, "outdoor_temp_task", 2048, strdup(&data[strlen(COMMAND_OUTDOOR_TEMP)]));
    }
}

/**
 * @brief Executes network based commands
 *
 * @param network
 * @param data
 */
void handle_network_command(uint8_t network, char *data)
{
    if (strcmp(data, COMMAND_PAIR_NET) == 0)
    {
        task_arg_t *args = calloc(1, sizeof(task_arg_t));
        args->network = network;
        args->args = strdup(&data[strlen(COMMAND_PAIR_NET)]);
        execute_task(network_pairing_task, "network_pairing_task", 4096, args);
    }
    else if (strcmp(data, COMMAND_DEVICE_STATUS) == 0)
    {
        uintptr_t network_arg = network;
        execute_task(device_status_task, "device_status_task", 4096, (uintptr_t *)network_arg);
    }
    else if (strcmp(data, COMMAND_DEVICE_STATUS_SHORT) == 0)
    {
        uintptr_t network_arg = network;
        execute_task(device_status_short_task, "device_status_short_task", 4096, (uintptr_t *)network_arg);
    }
}

/**
 * @brief Executes destination based commands
 *
 * @param network
 * @param target_mask
 * @param data
 */
void handle_dest_command(uint8_t network, uint16_t target_mask, char *data)
{
    task_arg_t *args = calloc(1, sizeof(task_arg_t));
    args->network = network;
    args->target_mask = target_mask;

    if (strcmp(data, COMMAND_PAIR) == 0)
    {
        if (no_of_devices(target_mask) > 1)
        {
            return;
        }
        execute_task(device_pairing_task, "device_pairing_task", 2048, args);
    }
    else if (strcmp(data, COMMAND_UNPAIR) == 0)
    {
        if (no_of_devices(target_mask) > 1)
        {
            return;
        }
        execute_task(unpairing_task, "unpairing_task", 2048, args);
    }
    else if (strncmp(data, COMMAND_READ, strlen(COMMAND_READ)) == 0)
    {
        args->args = strdup(&data[strlen(COMMAND_READ)]);
        execute_task(reading_task, "reading_task", 4096, args);
    }
    else if (strncmp(data, COMMAND_WRITE, strlen(COMMAND_WRITE)) == 0)
    {
        args->args = strdup(&data[strlen(COMMAND_WRITE)]);
        //execute_task(writing_task, "writing_task", 4096, args);
        free(args);
    }
    else if (strncmp(data, COMMAND_ENABLE, strlen(COMMAND_ENABLE)) == 0)
    {
        args->args = strdup(&data[strlen(COMMAND_ENABLE)]);
        execute_task(device_enable_task, "device_enable_task", 2048, args);
    }
    else if (strcmp(data, COMMAND_DISABLE) == 0)
    {
        execute_task(device_disable_task, "device_disable_task", 2048, args);
    }
    else
    {
        free(args);
    }
}

void handle_dest_topic(uint8_t network, uint16_t target_mask, char *topic, char *data)
{
    ESP_LOGI(TAG, "network: %d dest: %04x subtopic: %s", network, target_mask, topic);
    if (strcmp(topic, MQTT_TOPIC_CMD) == 0)
    {
        handle_dest_command(network, target_mask, data);
    }
}

/**
 * @brief
 *
 * @param network
 * @param topic
 * @param data
 */

void handle_network_topic(uint8_t network, char *topic, char *data)
{
    ESP_LOGI(TAG, "network: %d subtopic: %s", network, topic);
    if (strcmp(topic, MQTT_TOPIC_CMD) == 0)
    {
        handle_network_command(network, data);
    }
    else if (strncmp(topic, "/dest/", 6) == 0)
    {
        // move after dest;
        topic += 6;

        char *device_ids = strtok_r(topic, "/", &topic);
        if (strlen(device_ids) == 0)
        {
            return;
        }

        // parse list of device ids
        char* end = ",";
        uint16_t target_mask = 0;
        for (const char* p = device_ids; *end == ','; p = end + 1)
        {
            long target_number = strtol(p, &end, 10);
            if (end == p)
            {
                break;
            }
            if (target_number < 16)
            {
                target_mask |= 1 << target_number;
            }
        }

        if (target_mask == 0)
        {
            return;
        }

        // move one step back for sub topic and include leading slash, restore strtok_r change
        *(--topic) = '/';
        handle_dest_topic(network, target_mask, topic, data);
    }
}

/**
 * @brief MQTT Message data handler callback
 *
 * @param topic
 * @param data
 */
void mqtt_data(char *topic, char *data)
{
    // match device
    if (strncmp(topic, mqtt_topic_prefix, MQTT_TOPIC_PREFIX_LEN) != 0)
    {
        return;
    }

    // move pointer to remove prefix
    topic += MQTT_TOPIC_PREFIX_LEN;
    ESP_LOGI(TAG, "subtopic: %s", topic);
    if (strcmp(topic, MQTT_TOPIC_CMD) == 0)
    {
        handle_device_command(data);
    }
    else if (strncmp(topic, "/net-", 5) == 0)
    {
        // move after net;
        topic += 5;

        // parse network number
        uint8_t network = strtoul(topic, &topic, 10);
        if (!valid_network(network))
        {
            return;
        }

        handle_network_topic(network, topic, data);
    }
}

/**
 * @brief Callback from MQTT Handler on connect
 *
 */
void mqtt_connected(void)
{
    set_status(MQTT_STATUS_START);

    char topic[128];

    // device command topic
    snprintf(topic, 128, "%s%s", mqtt_topic_prefix, MQTT_TOPIC_CMD);
    mqtt_subscribe(topic, 0);

    // net 4 command topic
    snprintf(topic, 128, "%s/net-%d%s", mqtt_topic_prefix, NET_4, MQTT_TOPIC_CMD);
    mqtt_subscribe(topic, 0);

    // net 5 command topic
    snprintf(topic, 128, "%s/net-%d%s", mqtt_topic_prefix, NET_5, MQTT_TOPIC_CMD);
    mqtt_subscribe(topic, 0);

    // net 4 device command topic
    snprintf(topic, 128, "%s/net-%d/dest/+%s", mqtt_topic_prefix, NET_4, MQTT_TOPIC_CMD);
    mqtt_subscribe(topic, 0);

    // net 5 device command topic
    snprintf(topic, 128, "%s/net-%d/dest/+%s", mqtt_topic_prefix, NET_5, MQTT_TOPIC_CMD);
    mqtt_subscribe(topic, 0);

    // prepare destination device topics
    for (int i = 0; i < X3D_MAX_NET_DEVICES; i++)
    {
        publish_device(&net_4_devices[i], NET_4, i, true);
        publish_device(&net_5_devices[i], NET_5, i, true);
    }

    set_status(MQTT_STATUS_IDLE);
    led_color(0, 20, 0);
}

/**
 * @brief Main program entry
 *
 */
void app_main(void)
{
    // init OTA data
    ota_precheck();

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // setup LED
    led_init();

    // start WIFI
    ESP_ERROR_CHECK(wifi_init_sta());

    // execute OTA Update
    ota_execute();

    nvs_handle_t nvs_ctx_handle;
    if (nvs_open("ctx", NVS_READONLY, &nvs_ctx_handle) == ESP_OK)
    {
        x3d_device_type_t devices[X3D_MAX_NET_DEVICES] = { X3D_DEVICE_TYPE_NONE };
        size_t number_of_devices = X3D_MAX_NET_DEVICES;
        if (nvs_get_blob(nvs_ctx_handle, NVS_NET_4_DEVICES, &devices, &number_of_devices) == ESP_OK)
        {
            net_4_transfer_mask = init_device_data(devices, net_4_devices);
            ESP_LOGI(TAG, "Init Net 4 device data with mask 0x%04x", net_4_transfer_mask);
        }
        number_of_devices = X3D_MAX_NET_DEVICES;
        if (nvs_get_blob(nvs_ctx_handle, NVS_NET_5_DEVICES, &devices, &number_of_devices) == ESP_OK)
        {
            net_5_transfer_mask = init_device_data(devices, net_5_devices);
            ESP_LOGI(TAG, "Init Net 5 device data with mask 0x%04x", net_5_transfer_mask);
        }
        nvs_close(nvs_ctx_handle);
    }

    // load MAC address
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    // prepare device id
    x3d_set_device_id(mac[3] << 16 | mac[4] << 8 | mac[5]);
    snprintf(mqtt_topic_prefix, MQTT_TOPIC_PREFIX_SIZE, "device/x3d/%02x%02x%02x", mac[3], mac[4], mac[5]);
    snprintf(mqtt_topic_status, MQTT_TOPIC_STATUS_SIZE, "%s/status", mqtt_topic_prefix);

    // init RFM device
    ESP_ERROR_CHECK(rfm_init());

    // start MQTT
    mqtt_app_start(mqtt_topic_status, MQTT_STATUS_OFF, strlen(MQTT_STATUS_OFF), 0, 1);
}
