/**
 * esp32devkitc.h - ESP-IDF component to work with ESP32 DevkitC
 *
 * Include this header file to use the component.
 *
 * (C) 2019 - Timothee Cruse <timothee.cruse@gmail.com>
 * This code is licensed under the MIT License.
 */

#ifndef _ESP32DEVKITC_H_
#define _ESP32DEVKITC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"

#include "esp32devkitc_button.h"
#include "esp32devkitc_event.h"

/**
 * @brief   Initialize ESP32 DevkitC
 *
 *          Initializes buttons.
 *
 * @return  ESP_OK success
 *          ESP_FAIL errors found
 */
esp_err_t eESP32DevkitcInit( void );

#ifdef __cplusplus
}
#endif

#endif // _ESP32DEVKITC_H_
