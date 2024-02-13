/**
 * @file wifi.h
 * @author Sven Fabricius (sven.fabricius@livediesel.de)
 * @brief WIFI init functions
 * @version 0.1
 * @date 2023-11-15
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include "esp_system.h"
#include "esp_wifi.h"

esp_err_t wifi_init_sta(void);
