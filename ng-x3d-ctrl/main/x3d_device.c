/**
 * @file x3d_device.c
 * @author Sven Fabricius (sven.fabricius@livediesel.de)
 * @brief
 * @version 0.1
 * @date 2024-02-13
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <string.h>
#include "x3d_device.h"
#include "x3d.h"

#define FLAG_TO_BITFIELD(F, M)         (((F) & (M)) == (M))

// using string constants instead of defines to save flash memory
/*
static const char JSON_ACTION[] =               "action";
static const char JSON_NETWORK[] =              "net";
static const char JSON_ACK[] =                  "ack";
static const char JSON_REGISTER_HIGH[] =        "regHigh";
static const char JSON_REGISTER_LOW[] =         "regLow";
static const char JSON_VALUES[] =               "values";
static const char JSON_TARGET[] =               "target";
*/
static const char JSON_TYPE[] =                 "type";

static const char JSON_ROOM_TEMP[] =            "roomTemp";
static const char JSON_SET_POINT[] =            "setPoint";
static const char JSON_ENABLED[] =              "enabled";
static const char JSON_ON_AIR[] =               "onAir";
static const char JSON_SET_POINT_DAY[] =        "setPointDay";
static const char JSON_SET_POINT_NIGHT[] =      "setPointNight";
static const char JSON_SET_POINT_DEFROST[] =    "setPointDefrost";
static const char JSON_FLAGS[] =                "flags";
static const char JSON_POWER[] =                "power";

static const char JSON_DEFROST[] =              "defrost";
static const char JSON_TIMED[] =                "timed";
static const char JSON_HEATER_ON[] =            "heaterOn";
static const char JSON_HEATER_STOPPED[] =       "heaterStopped";
static const char JSON_WINDOW_OPEN[] =          "windowOpen";
static const char JSON_NO_TEMP_SENSOR[] =       "noTempSensor";
static const char JSON_BATTERY_LOW[] =          "batteryLow";

const char * x3d_device_type_to_string(x3d_device_type_t type)
{
    switch (type)
    {
        case X3D_DEVICE_TYPE_NONE: return "none";
        case X3D_DEVICE_TYPE_RF66XX: return "rf66xx";
    }
    return "unknown";
}

x3d_device_type_t x3d_device_type_from_string(const char *str)
{
    if (strcmp(str, "rf66xx") == 0)
    {
        return X3D_DEVICE_TYPE_RF66XX;
    }
    return X3D_DEVICE_TYPE_NONE;
}

bool x3d_device_from_type(x3d_device_t *device, x3d_device_type_t type)
{
    device->type = type;
    switch (type)
    {
        case X3D_DEVICE_TYPE_RF66XX:
            device->data = calloc(1, sizeof(x3d_rf66xx_t));
            return true;
        default:
            if (device->data != NULL)
            {
                free(device->data);
            }
            device->data = NULL;
            return false;
    }
}

cJSON * x3d_rf66xx_to_json(x3d_rf66xx_t *device)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, JSON_TYPE, x3d_device_type_to_string(X3D_DEVICE_TYPE_RF66XX));
    cJSON_AddNumberToObject(root, JSON_ROOM_TEMP, (double)device->room_temp / 100.0);
    cJSON_AddNumberToObject(root, JSON_POWER, (uint16_t)device->power * 50);
    cJSON_AddNumberToObject(root, JSON_SET_POINT, (double)device->set_point * 0.5);
    cJSON_AddNumberToObject(root, JSON_SET_POINT_DAY, (double)device->set_point_day * 0.5);
    cJSON_AddNumberToObject(root, JSON_SET_POINT_NIGHT, (double)device->set_point_night * 0.5);
    cJSON_AddNumberToObject(root, JSON_SET_POINT_DEFROST, (double)device->set_point_defrost * 0.5);
    cJSON_AddBoolToObject(root, JSON_ENABLED, device->enabled);
    cJSON_AddBoolToObject(root, JSON_ON_AIR, device->on_air);
    cJSON *flags = cJSON_AddArrayToObject(root, JSON_FLAGS);
    if (device->defrost)
    {
        cJSON_AddItemToArray(flags, cJSON_CreateString(JSON_DEFROST));
    }
    if (device->timed)
    {
        cJSON_AddItemToArray(flags, cJSON_CreateString(JSON_TIMED));
    }
    if (device->heater_on)
    {
        cJSON_AddItemToArray(flags, cJSON_CreateString(JSON_HEATER_ON));
    }
    if (device->heater_stopped)
    {
        cJSON_AddItemToArray(flags, cJSON_CreateString(JSON_HEATER_STOPPED));
    }
    if (device->window_open)
    {
        cJSON_AddItemToArray(flags, cJSON_CreateString(JSON_WINDOW_OPEN));
    }
    if (device->no_temp_sensor)
    {
        cJSON_AddItemToArray(flags, cJSON_CreateString(JSON_NO_TEMP_SENSOR));
    }
    if (device->battery_low)
    {
        cJSON_AddItemToArray(flags, cJSON_CreateString(JSON_BATTERY_LOW));
    }

    return root;
}

void x3d_rf66xx_set_from_reg(x3d_rf66xx_t *device, int req, int ack, uint16_t reg, uint16_t data)
{
    if (!req)
    {
        return;
    }

    if (!ack)
    {
        device->on_air = 0;
        return;
    }

    device->on_air = 1;
    switch (reg)
    {
    case X3D_REG_ATT_POWER:
        device->power = data & 0xff;
        break;
    case X3D_REG_ROOM_TEMP:
        device->room_temp = data;
        break;
    case X3D_REG_SETPOINT_STATUS:
        device->set_point      = data & 0xff;
        uint16_t flags         = data & 0xff00;
        device->defrost        = FLAG_TO_BITFIELD(flags, X3D_FLAG_DEFROST);
        device->timed          = FLAG_TO_BITFIELD(flags, X3D_FLAG_TIMED);
        device->heater_on      = FLAG_TO_BITFIELD(flags, X3D_FLAG_HEATER_ON);
        device->heater_stopped = FLAG_TO_BITFIELD(flags, X3D_FLAG_HEATER_STOPPED);
        break;
    case X3D_REG_ERROR_STATUS:
        device->window_open    = FLAG_TO_BITFIELD(data, X3D_FLAG_WINDOW_OPEN);
        device->no_temp_sensor = FLAG_TO_BITFIELD(data, X3D_FLAG_NO_TEMP_SENSOR);
        device->battery_low    = FLAG_TO_BITFIELD(data, X3D_FLAG_BATTERY_LOW);
        break;
    case X3D_REG_ON_OFF:
        device->enabled = FLAG_TO_BITFIELD(data, 0x0001);
        break;
    case X3D_REG_MODE_TIME:
        break;
    case X3D_REG_SETPOINT_DEFROST:
        device->set_point_defrost = data & 0xff;
        break;
    case X3D_REG_SETPOINT_NIGHT_DAY:
        device->set_point_night = data & 0xff;
        device->set_point_day   = (data >> 8) & 0xff;
        break;
    }
}