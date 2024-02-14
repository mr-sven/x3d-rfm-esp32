/**
 * @file mqtt.h
 * @author Sven Fabricius (sven.fabricius@livediesel.de)
 * @brief functions around mqtt client
 * @version 0.1
 * @date 2023-11-15
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include "mqtt_client.h"

void mqtt_app_start(char *lwt_topic, const char *lwt_data, int lwt_data_len, int qos, int retain);
int mqtt_publish(const char *topic, const char *data, int len, int qos, int retain);
int mqtt_subscribe(const char *topic, int qos);
int mqtt_subscribe_multi(const esp_mqtt_topic_t *topic_list, int size);