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

// using string constants instead of defines to save flash memory
static const char NVS_NET_4_DEVICES[] =              "net_4_devices";
static const char NVS_NET_5_DEVICES[] =              "net_5_devices";

static const char MQTT_TOPIC_RESET[] =               "/reset";
static const char MQTT_TOPIC_OUTDOOR_TEMP[] =        "/outdoor-temp";
static const char MQTT_TOPIC_PAIR[] =                "/pair";
static const char MQTT_TOPIC_DEVICE_STATUS[] =       "/device-status";
static const char MQTT_TOPIC_DEVICE_STATUS_SHORT[] = "/device-status-short";
static const char MQTT_TOPIC_READ[] =                "/read";
static const char MQTT_TOPIC_ENABLE[] =              "/enable";
static const char MQTT_TOPIC_DISABLE[] =             "/disable";
static const char MQTT_TOPIC_WRITE[] =               "/write";
static const char MQTT_TOPIC_UNPAIR[] =              "/unpair";

static const char MQTT_STATUS_OFF[] =                "off";
static const char MQTT_STATUS_IDLE[] =               "idle";
/*static const char MQTT_STATUS_PAIRING[] =            "pairing";
static const char MQTT_STATUS_PAIRING_SUCCESS[] =    "pairing success";
static const char MQTT_STATUS_PAIRING_FAILED[] =     "pairing failed";
static const char MQTT_STATUS_READING[] =            "reading";
static const char MQTT_STATUS_WRITING[] =            "writing";
static const char MQTT_STATUS_UNPAIRING[] =          "unpairing";
static const char MQTT_STATUS_STATUS[] =             "status"; */
static const char MQTT_STATUS_TEMP[] =               "temp";
static const char MQTT_STATUS_RESET[] =              "reset";
static const char MQTT_STATUS_START[] =              "start";

static const char TAG[] = "MAIN";

#define MQTT_TOPIC_PREFIX_LEN   18
#define MQTT_TOPIC_PREFIX_SIZE  (MQTT_TOPIC_PREFIX_LEN + 1)

#define MQTT_TOPIC_STATUS_LEN   25
#define MQTT_TOPIC_STATUS_SIZE  (MQTT_TOPIC_STATUS_LEN + 1)

// Prefix used for mqtt topics
static char mqtt_topic_prefix[MQTT_TOPIC_PREFIX_SIZE]; // strlen("/device/x3d/aabbcc") = 18 + 1

// status topic
static char mqtt_topic_status[MQTT_TOPIC_STATUS_SIZE]; // strlen("/device/x3d/aabbcc/status") = 25 + 1

// list of devices
static x3d_device_t net_4_devices[X3D_MAX_NET_DEVICES] = {0};
static x3d_device_t net_5_devices[X3D_MAX_NET_DEVICES] = {0};

// transfer masks
static uint16_t net_4_transfer_mask = 0;
static uint16_t net_5_transfer_mask = 0;

TaskHandle_t processing_task_handle = NULL;

static inline int valid_network(uint8_t network)
{
    return network == NET_4 || network == NET_5;
}

/**
 * @brief Initialize device data list
 *
 * @param nvs_devices
 * @param devices
 * @return uint16_t device transfer mask
 */
uint16_t init_device_data(x3d_device_type_t *nvs_devices, x3d_device_t devices[X3D_MAX_NET_DEVICES])
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
uint16_t get_target_mask_by_feature(x3d_device_t devices[X3D_MAX_NET_DEVICES], x3d_device_feature_t feature)
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


void handle_dest_topic(uint8_t network, uint16_t target_mask, char *topic, char *data)
{
    ESP_LOGI(TAG, "network: %d dest: 0x%04x subtopic: %s", network, target_mask, topic);
    if (strcmp(topic, MQTT_TOPIC_PAIR) == 0)
    {

    }
    else if (strcmp(topic, MQTT_TOPIC_UNPAIR) == 0)
    {

    }
    else if (strcmp(topic, MQTT_TOPIC_READ) == 0)
    {

    }
    else if (strcmp(topic, MQTT_TOPIC_WRITE) == 0)
    {

    }
    else if (strcmp(topic, MQTT_TOPIC_ENABLE) == 0)
    {

    }
    else if (strcmp(topic, MQTT_TOPIC_DISABLE) == 0)
    {

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
    if (strcmp(topic, MQTT_TOPIC_PAIR) == 0)
    {

    }
    else if (strcmp(topic, MQTT_TOPIC_DEVICE_STATUS) == 0)
    {

    }
    else if (strcmp(topic, MQTT_TOPIC_DEVICE_STATUS_SHORT) == 0)
    {

    }
    else if (strcmp(topic, MQTT_TOPIC_READ) == 0)
    {

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
    if (strcmp(topic, MQTT_TOPIC_RESET) == 0)
    {
        set_status(MQTT_STATUS_RESET);
        ESP_LOGI(TAG, "Prepare to restart system!");
        esp_restart();
    }
    else if (strcmp(topic, MQTT_TOPIC_OUTDOOR_TEMP) == 0)
    {
        execute_task(outdoor_temp_task, "outdoor_temp_task", 2048, strdup(data));
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

    // device based topics
    mqtt_subscribe_subtopic(MQTT_TOPIC_RESET);
    mqtt_subscribe_subtopic(MQTT_TOPIC_OUTDOOR_TEMP);

    // network based topics
    for (int i = NET_4; i <= NET_5; i++)
    {
        mqtt_subscribe_net_subtopic(MQTT_TOPIC_PAIR, i);
        mqtt_subscribe_net_subtopic(MQTT_TOPIC_DEVICE_STATUS, i);
        mqtt_subscribe_net_subtopic(MQTT_TOPIC_DEVICE_STATUS_SHORT, i);
        mqtt_subscribe_net_subtopic(MQTT_TOPIC_READ, i);
        mqtt_subscribe_net_device_subtopic(MQTT_TOPIC_READ, i);
        mqtt_subscribe_net_device_subtopic(MQTT_TOPIC_WRITE, i);
        mqtt_subscribe_net_device_subtopic(MQTT_TOPIC_ENABLE, i);
        mqtt_subscribe_net_device_subtopic(MQTT_TOPIC_DISABLE, i);
        mqtt_subscribe_net_device_subtopic(MQTT_TOPIC_PAIR, i);
        mqtt_subscribe_net_device_subtopic(MQTT_TOPIC_UNPAIR, i);
    }

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
    snprintf(mqtt_topic_prefix, MQTT_TOPIC_PREFIX_SIZE, "/device/x3d/%02x%02x%02x", mac[3], mac[4], mac[5]);
    snprintf(mqtt_topic_status, MQTT_TOPIC_STATUS_SIZE, "%s/status", mqtt_topic_prefix);

    // init RFM device
    ESP_ERROR_CHECK(rfm_init());

    // start MQTT
    mqtt_app_start(mqtt_topic_status, MQTT_STATUS_OFF, strlen(MQTT_STATUS_OFF), 0, 1);
}
