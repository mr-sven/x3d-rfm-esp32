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

#define MQTT_STATUS_OFF                "off"
#define MQTT_STATUS_IDLE               "idle"
#define MQTT_STATUS_PAIRING            "pairing"
#define MQTT_STATUS_PAIRING_SUCCESS    "pairing success"
#define MQTT_STATUS_PAIRING_FAILED     "pairing failed"
#define MQTT_STATUS_READING            "reading"
#define MQTT_STATUS_WRITING            "writing"
#define MQTT_STATUS_UNPAIRING          "unpairing"
#define MQTT_STATUS_TEMP               "temp"
#define MQTT_STATUS_STATUS             "status"
#define MQTT_STATUS_RESET              "reset"
#define MQTT_STATUS_START              "start"



static const char *TAG = "MAIN";

static char mqtt_topic_prefix[19]; // strlen("/device/x3d/aabbcc") = 18 + 1
static char mqtt_topic_status[26]; // strlen("/device/x3d/aabbcc/status") = 25 + 1


/**
 * @brief Set the status of the controller
 *
 * @param status
 */
void set_status(char *status)
{
    mqtt_publish(mqtt_topic_status, status, strlen(status), 0, 1);
}





/**
 * @brief MQTT Message data handler callback
 *
 * @param topic
 * @param data
 */
void mqtt_data(char *topic, char *data)
{
    if (strncmp(mqtt_topic_prefix, topic, strlen(mqtt_topic_prefix)) != 0)
    {
        return;
    }

    // TODO: Handle MQTT topics
}

/**
 * @brief Callback from MQTT Handler on connect
 *
 */
void mqtt_connected(void)
{
    // TODO: Subscribe topics

    set_status(MQTT_STATUS_START);
    set_status(MQTT_STATUS_IDLE);

    // TODO: reset destination device topics

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

    // TODO: Load destination device data from nvs

    // load MAC address
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    // prepare device id
    x3d_set_device_id(mac[3] << 16 | mac[4] << 8 | mac[5]);
    sprintf(mqtt_topic_prefix, "/device/x3d/%02x%02x%02x", mac[3], mac[4], mac[5]);
    sprintf(mqtt_topic_status, "%s/status", mqtt_topic_prefix);

    // TODO: init device cache

    // init RFM device
    ESP_ERROR_CHECK(rfm_init());

    // start MQTT
    mqtt_app_start(mqtt_topic_status, MQTT_STATUS_OFF, strlen(MQTT_STATUS_OFF), 0, 1);
}
