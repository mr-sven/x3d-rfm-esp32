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
#include "esp_random.h"
#include "esp_timer.h"

#include "nvs_flash.h"
#include "cJSON.h"

#include "config.h"
#include "wifi.h"
#include "rfm.h"
#include "mqtt.h"
#include "x3d.h"
#include "led.h"

#define MQTT_TOPIC_PREFIX_LEN             20
#define MQTT_TOPIC_STATUS                 "/status"
#define MQTT_TOPIC_RESULT                 "/result"

#define MQTT_STATUS_IDLE                  "idle"
#define MQTT_STATUS_PAIRING               "pairing"
#define MQTT_STATUS_PAIRING_SUCCESS       "pairing success"
#define MQTT_STATUS_PAIRING_FAILED        "pairing failed"
#define MQTT_STATUS_READING               "reading"
#define MQTT_STATUS_WRITING               "writing"
#define MQTT_STATUS_UNPAIRING             "unpairing"

#define MQTT_TOPIC_CMD                    "/cmd"
#define MQTT_CMD_PAIR                     "pair"
#define MQTT_CMD_READ                     "read"
#define MQTT_CMD_WRITE                    "write"
#define MQTT_CMD_UNPAIR                   "unpair"

#define MQTT_JSON_ACTION                  "action"
#define MQTT_JSON_NETWORK                 "net"
#define MQTT_JSON_NO_OF_DEVICES           "noOfDevices"
#define MQTT_JSON_ACK                     "ack"
#define MQTT_JSON_REGISTER_HIGH           "regHigh"
#define MQTT_JSON_REGISTER_LOW            "regLow"
#define MQTT_JSON_VALUES                  "values"
#define MQTT_JSON_TRANSFER                "transfer"
#define MQTT_JSON_TARGET                  "target"

#define TRANSFER_SLOT_MASK(X)    ((1 << X) - 1)

typedef struct {
    uint8_t network;
    uint8_t no_of_devices;
} x3d_pairing_data_t;

typedef struct {
    uint8_t network;
    uint16_t transfer;
    uint16_t target;
} x3d_unpairing_data_t;

typedef struct {
    uint8_t network;
    uint16_t transfer;
    uint16_t target;
    uint8_t register_high;
    uint8_t register_low;
} x3d_read_data_t;

typedef struct {
    uint8_t network;
    uint16_t transfer;
    uint16_t target;
    uint8_t register_high;
    uint8_t register_low;
    uint16_t values[X3D_MAX_PAYLOAD_DATA_FIELDS];
} x3d_write_data_t;

static const char *TAG = "MAIN";

static char *mqtt_topic_prefix = NULL;
static char *mqtt_status_topic = NULL;
static char *mqtt_result_topic = NULL;

static uint32_t x3d_device_id;
static uint8_t x3d_buffer[64];
static uint8_t x3d_msg_no = 1;
static uint16_t x3d_msg_id = 1;
TaskHandle_t x3d_processing_task_handle = NULL;

static inline int get_highest_bit(uint16_t value)
{
    return sizeof(unsigned int) * 8 - __builtin_clz(value) - 1;
}

void set_status(char *status)
{
    mqtt_publish(mqtt_status_topic, status, strlen(status), 0, 1);
}

void x3d_processor(uint8_t *buffer)
{
    //ESP_LOG_BUFFER_HEX_LEVEL(TAG, buffer, buffer[0], ESP_LOG_INFO);
    /*
     * It is possible to make all checks by hand:
     *  * message number
     *  * message type
     *  * message id
     *  * device id
     *  * header checksum
     * It is easier and faster to do a memcmp over the whole message header, because it will always returned 1 to 1 by the responding device.
     */
    // get length of the request message header
    uint8_t check_length = (x3d_buffer[X3D_IDX_HEADER_LEN] & X3D_HEADER_LENGTH_MASK) + X3D_IDX_HEADER_LEN;

    // if header does not match, ignore
    if (memcmp(x3d_buffer, buffer, check_length) != 0)
    {
        return;
    }

    /*
     * I don't know how the data processing is done in deltadore devices, here is how I do it.
     * Every mesh device has its own bits in bitfields and 16bit data response slots, so we can assume the following.
     * As long as the retry value is greater than the current one, we can bitwise or the whole payload except the retry value.
     * In general all mesh device update their payloads so the the highest retry value packet should contain all data.
     * But if the Mesh is cut in half, so that not all devices can communicate to all and only the station has the whole connectivity, it could
     * be possible that some data is missing, so the following process continiously updates the data.
     *
     * There may be a gap, for example in pairing process the paring pin is also a shared 16bit field, but if more than one device is in pairing,
     * then all devices return the same retry count value, so the message with the overlapping pin should be ignored.
     */

    // payload index is the end of the header check
    uint8_t payload_index = check_length;

    // check if retry count lower than the received
    if (x3d_buffer[payload_index] >= buffer[payload_index])
    {
        return;
    }

    // update retry count
    x3d_buffer[payload_index] = buffer[payload_index];

    // step over retry count, walk to the end
    for (payload_index++; payload_index < x3d_buffer[0]; payload_index++)
    {
        // orwise bytes to the buffer
        x3d_buffer[payload_index] |= buffer[payload_index];
    }
}

static uint8_t x3d_prepare_message(uint8_t network, x3d_msg_type_t msg_type, uint8_t flags, uint8_t status, uint8_t *ext_header, int ext_header_len)
{
    x3d_init_message(x3d_buffer, x3d_device_id, 0x80 | network);
    return x3d_prepare_message_header(x3d_buffer, &x3d_msg_no, msg_type, flags, status, ext_header, ext_header_len, x3d_enc_msg_id(&x3d_msg_id, x3d_device_id));
}

static void x3d_transmit(void)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    do
    {
        x3d_set_crc(x3d_buffer);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(X3D_MSG_DELAY_MS));
        rfm_transfer(x3d_buffer, x3d_buffer[0]);
    } while (x3d_dec_retry(x3d_buffer) > 0);
    rfm_receive();
}

static void x3d_pairing_task(void *arg)
{
    set_status(MQTT_STATUS_PAIRING);
    x3d_pairing_data_t *pairing_data = (x3d_pairing_data_t *)arg;

    uint8_t ext_header[] = {0x98, 0x00};
    uint8_t payload_index = x3d_prepare_message(pairing_data->network, X3D_MSG_TYPE_PAIRING, 0, 0x85, ext_header, sizeof(ext_header));
    uint16_t ack_mask = 1 << pairing_data->no_of_devices;
    uint16_t transfer_slot = ack_mask - 1;

    x3d_set_message_retrans(x3d_buffer, payload_index, 4, transfer_slot);
    x3d_set_pairing_data(x3d_buffer, payload_index, pairing_data->no_of_devices, 0, X3D_PAIR_STATE_OPEN);

    // transfer buffer
    x3d_transmit();

    // wait to process responses
    vTaskDelay(pdMS_TO_TICKS(5000));

    uint16_t pin = x3d_get_pairing_pin(x3d_buffer, payload_index);
    if (pin != 0)
    {
        payload_index = x3d_prepare_message(pairing_data->network, X3D_MSG_TYPE_PAIRING, 0, 0x85, ext_header, sizeof(ext_header));
        x3d_set_message_retrans(x3d_buffer, payload_index, 4, transfer_slot);
        x3d_set_pairing_data(x3d_buffer, payload_index, pairing_data->no_of_devices, pin, X3D_PAIR_STATE_PINNED);

        // transfer buffer
        x3d_transmit();

        // wait to process responses
        vTaskDelay(pdMS_TO_TICKS((pairing_data->no_of_devices + 1) * 4 * X3D_MSG_DELAY_MS));

        if ((x3d_get_retrans_ack(x3d_buffer, payload_index) & ack_mask) == ack_mask)
        {
            set_status(MQTT_STATUS_PAIRING_SUCCESS);
        }
        else
        {
            set_status(MQTT_STATUS_PAIRING_FAILED);
        }
    }
    else
    {
        set_status(MQTT_STATUS_PAIRING_FAILED);
    }

    free(pairing_data);
    set_status(MQTT_STATUS_IDLE);
    x3d_processing_task_handle = NULL;
    vTaskDelete(NULL);
}

void process_pairing(cJSON *cmd)
{
    if (x3d_processing_task_handle != NULL)
    {
        ESP_LOGE(TAG, "X3D message processing in progress");
        return;
    }

    cJSON *network = cJSON_GetObjectItem(cmd, MQTT_JSON_NETWORK);
    cJSON *no_of_devices = cJSON_GetObjectItem(cmd, MQTT_JSON_NO_OF_DEVICES);

    if (network == NULL || no_of_devices == NULL || !cJSON_IsNumber(network) || !cJSON_IsNumber(no_of_devices))
    {
        return;
    }

    x3d_pairing_data_t *pairing_data = (x3d_pairing_data_t *)malloc(sizeof(x3d_pairing_data_t));
    pairing_data->network = network->valueint;
    pairing_data->no_of_devices = no_of_devices->valueint;
    ESP_LOGI(TAG, "process_pairing network %d devices %d", pairing_data->network, pairing_data->no_of_devices);
    xTaskCreate(x3d_pairing_task, "x3d_pairing_task", 2048, pairing_data, 10, &x3d_processing_task_handle);
}

static void x3d_reading_task(void* arg)
{
    set_status(MQTT_STATUS_READING);
    x3d_read_data_t *read_data = (x3d_read_data_t *)arg;

    uint8_t ext_header[] = {0x98, 0x00};
    uint8_t payload_index = x3d_prepare_message(read_data->network, X3D_MSG_TYPE_STANDARD, 0, 0x05, ext_header, sizeof(ext_header));
    x3d_set_message_retrans(x3d_buffer, payload_index, 4, read_data->transfer);
    x3d_set_register_read(x3d_buffer, payload_index, read_data->target, read_data->register_high, read_data->register_low);

    int no_of_devices = get_highest_bit(read_data->transfer) + 1;

    // transfer buffer
    x3d_transmit();

    // wait to process responses
    vTaskDelay(pdMS_TO_TICKS(no_of_devices * 4 * X3D_MSG_DELAY_MS));

    x3d_standard_msg_payload_p payload = (x3d_standard_msg_payload_p)&x3d_buffer[payload_index + 1];
    ESP_LOGI(TAG, "read register %02x - %02x from %04x", payload->reg_high, payload->reg_low, payload->target_ack);

    cJSON *root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, MQTT_JSON_NETWORK, read_data->network);
	cJSON_AddNumberToObject(root, MQTT_JSON_ACK, payload->target_ack);
	cJSON_AddNumberToObject(root, MQTT_JSON_REGISTER_HIGH, payload->reg_high);
	cJSON_AddNumberToObject(root, MQTT_JSON_REGISTER_LOW, payload->reg_low);
    cJSON *values = cJSON_AddArrayToObject(root, MQTT_JSON_VALUES);
    for (int i = 0; i < no_of_devices; i++)
    {
	    cJSON_AddItemToArray(values, cJSON_CreateNumber(payload->data[i]));
    }
    char *json_string = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

    mqtt_publish(mqtt_result_topic, json_string, strlen(json_string), 0, 0);
    free(json_string);

    free(read_data);
    set_status(MQTT_STATUS_IDLE);
    x3d_processing_task_handle = NULL;
    vTaskDelete(NULL);
}

void process_reading(cJSON *cmd)
{
    if (x3d_processing_task_handle != NULL)
    {
        ESP_LOGE(TAG, "X3D message processing in progress");
        return;
    }

    cJSON *network = cJSON_GetObjectItem(cmd, MQTT_JSON_NETWORK);
    cJSON *transfer = cJSON_GetObjectItem(cmd, MQTT_JSON_TRANSFER);
    cJSON *target = cJSON_GetObjectItem(cmd, MQTT_JSON_TARGET);
    cJSON *register_high = cJSON_GetObjectItem(cmd, MQTT_JSON_REGISTER_HIGH);
    cJSON *register_low = cJSON_GetObjectItem(cmd, MQTT_JSON_REGISTER_LOW);
    if (network == NULL || transfer == NULL || target == NULL || register_high == NULL || register_low == NULL ||
        !cJSON_IsNumber(network) || !cJSON_IsNumber(transfer) || !cJSON_IsNumber(target) || !cJSON_IsNumber(register_high) || !cJSON_IsNumber(register_low))
    {
        return;
    }

    x3d_read_data_t *read_data = (x3d_read_data_t *)malloc(sizeof(x3d_read_data_t));
    read_data->network = network->valueint;
    read_data->transfer = transfer->valueint;
    read_data->target = target->valueint;
    read_data->register_high = register_high->valueint;
    read_data->register_low = register_low->valueint;

    ESP_LOGI(TAG, "process_reading network %d via %04x from %04x register %02x - %02x", read_data->network, read_data->transfer, read_data->target, read_data->register_high, read_data->register_low);
    xTaskCreate(x3d_reading_task, "x3d_reading_task", 2048, read_data, 10, &x3d_processing_task_handle);
}

static void x3d_writing_task(void* arg)
{
    set_status(MQTT_STATUS_WRITING);
    x3d_write_data_t *write_data = (x3d_write_data_t *)arg;

    uint8_t ext_header[] = {0x98, 0x00};
    uint8_t payload_index = x3d_prepare_message(write_data->network, X3D_MSG_TYPE_STANDARD, 0, 0x05, ext_header, sizeof(ext_header));
    x3d_set_message_retrans(x3d_buffer, payload_index, 4, write_data->transfer);
    x3d_set_register_write(x3d_buffer, payload_index, write_data->target, write_data->register_high, write_data->register_low, write_data->values);

    int no_of_devices = get_highest_bit(write_data->transfer) + 1;

    // transfer buffer
    x3d_transmit();

    // wait to process responses
    vTaskDelay(pdMS_TO_TICKS(no_of_devices * 4 * X3D_MSG_DELAY_MS));

    x3d_standard_msg_payload_p payload = (x3d_standard_msg_payload_p)&x3d_buffer[payload_index + 1];
    ESP_LOGI(TAG, "write register %02x - %02x to %04x", payload->reg_high, payload->reg_low, payload->target_ack);

    cJSON *root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, MQTT_JSON_NETWORK, write_data->network);
	cJSON_AddNumberToObject(root, MQTT_JSON_ACK, payload->target_ack);
	cJSON_AddNumberToObject(root, MQTT_JSON_REGISTER_HIGH, payload->reg_high);
	cJSON_AddNumberToObject(root, MQTT_JSON_REGISTER_LOW, payload->reg_low);
    cJSON *values = cJSON_AddArrayToObject(root, MQTT_JSON_VALUES);
    for (int i = 0; i < no_of_devices; i++)
    {
	    cJSON_AddItemToArray(values, cJSON_CreateNumber(payload->data[i]));
    }
    char *json_string = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

    mqtt_publish(mqtt_result_topic, json_string, strlen(json_string), 0, 0);
    free(json_string);

    free(write_data);
    set_status(MQTT_STATUS_IDLE);
    x3d_processing_task_handle = NULL;
    vTaskDelete(NULL);
}

void process_writing(cJSON *cmd)
{
    if (x3d_processing_task_handle != NULL)
    {
        ESP_LOGE(TAG, "X3D message processing in progress");
        return;
    }

    cJSON *network = cJSON_GetObjectItem(cmd, MQTT_JSON_NETWORK);
    cJSON *transfer = cJSON_GetObjectItem(cmd, MQTT_JSON_TRANSFER);
    cJSON *target = cJSON_GetObjectItem(cmd, MQTT_JSON_TARGET);
    cJSON *register_high = cJSON_GetObjectItem(cmd, MQTT_JSON_REGISTER_HIGH);
    cJSON *register_low = cJSON_GetObjectItem(cmd, MQTT_JSON_REGISTER_LOW);
    cJSON *values = cJSON_GetObjectItem(cmd, MQTT_JSON_VALUES);

    if (network == NULL || transfer == NULL || target == NULL || register_high == NULL || register_low == NULL || values == NULL ||
        !cJSON_IsNumber(network) || !cJSON_IsNumber(transfer) || !cJSON_IsNumber(target) || !cJSON_IsNumber(register_high) || !cJSON_IsNumber(register_low) || cJSON_GetArraySize(values) < 1)
    {
        return;
    }

    x3d_write_data_t *write_data = (x3d_write_data_t *)malloc(sizeof(x3d_write_data_t));
    write_data->network = network->valueint;
    write_data->transfer = transfer->valueint;
    write_data->target = target->valueint;
    write_data->register_high = register_high->valueint;
    write_data->register_low = register_low->valueint;
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
            write_data->values[i] = value->valueint;
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
            write_data->values[i] = value->valueint;
        }
    }

    ESP_LOGI(TAG, "process_writing network %d via %04x to %04x register %02x - %02x", write_data->network, write_data->transfer, write_data->target, write_data->register_high, write_data->register_low);
    xTaskCreate(x3d_writing_task, "x3d_writing_task", 2048, write_data, 10, &x3d_processing_task_handle);
}

static void x3d_unpairing_task(void* arg)
{
    set_status(MQTT_STATUS_UNPAIRING);
    x3d_unpairing_data_t *unpairing_data = (x3d_unpairing_data_t *)arg;

    uint8_t ext_header[] = {0x98, 0x00};
    uint8_t payload_index = x3d_prepare_message(unpairing_data->network, X3D_MSG_TYPE_STANDARD, 0, 0x05, ext_header, sizeof(ext_header));
    x3d_set_message_retrans(x3d_buffer, payload_index, 4, unpairing_data->transfer);
    x3d_set_unpair_device(x3d_buffer, payload_index, unpairing_data->target);

    int no_of_devices = get_highest_bit(unpairing_data->transfer) + 1;

    // transfer buffer
    x3d_transmit();

    // wait to process responses
    vTaskDelay(pdMS_TO_TICKS(no_of_devices * 4 * X3D_MSG_DELAY_MS));

    free(unpairing_data);
    set_status(MQTT_STATUS_IDLE);
    x3d_processing_task_handle = NULL;
    vTaskDelete(NULL);
}

void process_unpairing(cJSON *cmd)
{
    if (x3d_processing_task_handle != NULL)
    {
        ESP_LOGE(TAG, "X3D message processing in progress");
        return;
    }

    cJSON *network = cJSON_GetObjectItem(cmd, MQTT_JSON_NETWORK);
    cJSON *transfer = cJSON_GetObjectItem(cmd, MQTT_JSON_TRANSFER);
    cJSON *target = cJSON_GetObjectItem(cmd, MQTT_JSON_TARGET);

    if (network == NULL || transfer == NULL || target == NULL || !cJSON_IsNumber(network) || !cJSON_IsNumber(transfer) || !cJSON_IsNumber(target))
    {
        return;
    }

    x3d_unpairing_data_t *unpairing_data = (x3d_unpairing_data_t *)malloc(sizeof(x3d_unpairing_data_t));
    unpairing_data->network = network->valueint;
    unpairing_data->transfer = transfer->valueint;
    unpairing_data->target = target->valueint;
    ESP_LOGI(TAG, "process_unpairing network %d via %04x to %04x", unpairing_data->network, unpairing_data->transfer, unpairing_data->target);
    xTaskCreate(x3d_unpairing_task, "x3d_unpairing_task", 2048, unpairing_data, 10, &x3d_processing_task_handle);
}

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

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    // prepare device id
    x3d_device_id = mac[3] << 16 | mac[4] << 8 | mac[5];
    mqtt_topic_prefix = calloc(MQTT_TOPIC_PREFIX_LEN + 1, sizeof(char));
    sprintf(mqtt_topic_prefix, "/device/esp32/%02x%02x%02x", mac[3], mac[4], mac[5]);

    // prepare status topic
    mqtt_status_topic = calloc(strlen(mqtt_topic_prefix) + strlen(MQTT_TOPIC_STATUS) + 1, sizeof(char));
    strcpy (mqtt_status_topic, mqtt_topic_prefix);
    strcat (mqtt_status_topic, MQTT_TOPIC_STATUS);

    // prepare status topic
    mqtt_result_topic = calloc(strlen(mqtt_topic_prefix) + strlen(MQTT_TOPIC_RESULT) + 1, sizeof(char));
    strcpy (mqtt_result_topic, mqtt_topic_prefix);
    strcat (mqtt_result_topic, MQTT_TOPIC_RESULT);

    ESP_ERROR_CHECK(rfm_init());
    ESP_ERROR_CHECK(wifi_init_sta());
    mqtt_app_start(mqtt_status_topic, "off", 3, 0, 1);
}
