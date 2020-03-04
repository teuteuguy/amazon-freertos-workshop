/**
 * esp32devkitc.c - ESP-IDF component to work with ESP32 DevkitC
 *
 * (C) 2020 - Timothee Cruse <timothee.cruse@gmail.com>
 * This code is licensed under the MIT License.
 */

#include "esp32devkitc.h"

static const char *TAG = "esp32devkitc";

esp_err_t eESP32DevkitcInit( void )
{
    esp_err_t res;
    res = eESP32DevkitcEventInit();

    if(res == ESP_OK) {
        ESP_LOGD(TAG, "Event initialized");
    } else {
        ESP_LOGE(TAG, "Error initializing event");
        return res;
    }

    // Init button
    res = eESP32DevkitcButtonInit();
    if(res == ESP_OK) {
        ESP_LOGD(TAG, "Button initialized");
    } else {
        ESP_LOGE(TAG, "Error initializing button");
        return res;
    }

    return res;
}
