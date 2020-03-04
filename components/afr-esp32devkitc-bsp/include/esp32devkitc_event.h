/**
 * esp32devkitc_event.h
 *
 * (C) 2020 - Timothee Cruse <timothee.cruse@gmail.com>
 * This code is licensed under the MIT License.
 */

#ifndef _ESP32DEVKITC_EVENT_H_
#define _ESP32DEVKITC_EVENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_log.h"
#include "esp_event.h"

extern esp_event_loop_handle_t esp32devkitc_event_loop;   /*!< Event loop for ESP32 DevkitC device-specific events */

esp_err_t eESP32DevkitcEventInit( void );

#ifdef __cplusplus
}
#endif

#endif // _ESP32DEVKITC_EVENT_H_
