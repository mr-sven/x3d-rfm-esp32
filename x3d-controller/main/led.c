/**
 * @file led.c
 * @author Sven Fabricius (sven.fabricius@livediesel.de)
 * @brief
 * @version 0.1
 * @date 2023-12-04
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "freertos/FreeRTOS.h"

#include "esp_system.h"
#include "driver/ledc.h"

#include "led.h"
#include "config.h"

#define RGB_TO_DUTY(x)  (x * (1 << LEDC_DUTY_RES) / 255)

void led_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 4 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ledc_channel.channel        = LEDC_RED_CHANNEL;
    ledc_channel.gpio_num       = LEDC_RED_OUTPUT;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    ledc_channel.channel        = LEDC_GREEN_CHANNEL;
    ledc_channel.gpio_num       = LEDC_GREEN_OUTPUT;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    ledc_channel.channel        = LEDC_BLUE_CHANNEL;
    ledc_channel.gpio_num       = LEDC_BLUE_OUTPUT;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    led_color(0xff, 0, 0);
}

void led_color(uint8_t red, uint8_t green, uint8_t blue)
{
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_RED_CHANNEL, RGB_TO_DUTY((0xff - red))));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_RED_CHANNEL));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_GREEN_CHANNEL, RGB_TO_DUTY((0xff - green))));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_GREEN_CHANNEL));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_BLUE_CHANNEL, RGB_TO_DUTY((0xff - blue))));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_BLUE_CHANNEL));
}