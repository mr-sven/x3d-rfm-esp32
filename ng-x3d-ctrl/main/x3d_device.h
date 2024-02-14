/**
 * @file x3d_device.h
 * @author Sven Fabricius (sven.fabricius@livediesel.de)
 * @brief destination device type definitions
 * @version 0.1
 * @date 2024-02-13
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#include <stdbool.h>
#include <inttypes.h>
#include "cJSON.h"

// Type of X3D device
typedef enum __attribute__ ((__packed__)) {
    X3D_DEVICE_TYPE_NONE = 0,
    X3D_DEVICE_TYPE_RF66XX = 1,
} x3d_device_type_t;
_Static_assert(sizeof(x3d_device_type_t) == 1); // make shure it's 1 byte long, coule be extended if device types overflow 255

// list of features
typedef enum {
    X3D_DEVICE_FEATURE_OUTDOOR_TEMP = 0x01,
    X3D_DEVICE_FEATURE_TEMP_ACTOR =  0x02,
} x3d_device_feature_t;

// list of fatures per device
static const x3d_device_feature_t x3d_device_feature_list[] = {
    // X3D_DEVICE_TYPE_NONE
    0,
    // X3D_DEVICE_TYPE_RF66XX
    X3D_DEVICE_FEATURE_OUTDOOR_TEMP | X3D_DEVICE_FEATURE_TEMP_ACTOR,
};

// Device data type
typedef struct
{
    x3d_device_type_t type;
    void *data;
} x3d_device_t;

typedef struct
{
    uint16_t room_temp;
    uint8_t power;
    uint8_t set_point;
    uint8_t set_point_day;
    uint8_t set_point_night;
    uint8_t set_point_defrost;
    int on_air : 1;
    int enabled : 1;
    int defrost : 1;
    int timed : 1;
    int heater_on : 1;
    int heater_stopped : 1;
    int window_open : 1;
    int no_temp_sensor : 1;
    int battery_low : 1;
} x3d_rf66xx_t;

/**
 * @brief convert type to string
 *
 * @param type devic etype
 * @return const char*
 */
const char * x3d_device_type_to_string(x3d_device_type_t type);

/**
 * @brief sets device type struct from type
 *
 * @param device pointer to target struct
 * @param type type of device
 * @return bool true if device is valisd
 */
bool x3d_device_from_type(x3d_device_t *device, x3d_device_type_t type);

/**
 * @brief Converts x3d_rf66xx_t device to json object
 *
 * @param device
 * @return cJSON*
 */
cJSON * x3d_rf66xx_to_json(x3d_rf66xx_t *device);