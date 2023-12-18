/**
 * @file ota.h
 * @author Sven Fabricius (sven.fabricius@livediesel.de)
 * @brief
 * @version 0.1
 * @date 2023-12-13
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define OTA_DONE_BIT        BIT0

extern EventGroupHandle_t ota_event_group;

void ota_precheck(void);
void ota_update_task(void *arg);