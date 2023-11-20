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

#include "esp_system.h"

#include "rfm.h"

void app_main(void)
{
    ESP_ERROR_CHECK(rfm_init());
}
