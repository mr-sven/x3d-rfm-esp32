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

void mqtt_app_start(void);
int mqtt_publish(const char *topic, const char *data, int len, int qos, int retain);