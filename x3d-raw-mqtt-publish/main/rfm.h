/**
 * @file rfm.h
 * @author Sven Fabricius (sven.fabricius@livediesel.de)
 * @brief RFM96 / SX1231 setup functions
 * @version 0.1
 * @date 2023-11-15
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include "esp_system.h"

esp_err_t rfm_init(void);