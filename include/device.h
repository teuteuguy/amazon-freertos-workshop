/**
 * @file device.h
 *
 * (C) 2019 - Timothee Cruse <timothee.cruse@gmail.com>
 * This code is licensed under the MIT License.
 */

#ifndef _DEVICE_H_
#define _DEVICE_H_

#include "esp_err.h"

#include "lab_config.h"

#define STATUS_LED_ON()
#define STATUS_LED_OFF()

#define DISPLAY_PRINT(str, x, y)
#define DISPLAY_WIDTH
#define DISPLAY_HEIGHT

#define BUTTON_CLICK    0
#define BUTTON_HOLD     1

#if defined(DEVICE_ESP32_DEVKITC)

    #include "esp32devkitc.h"

    #define DEVICE_HAS_MAIN_BUTTON
    #define BUTTON_MAIN_EVENT_BASE ESP32DEVKITC_BUTTON_EVENT_BASE

#elif defined(DEVICE_M5STICKC)
    
    #include "m5stickc.h"

    #define DEVICE_HAS_ACCELEROMETER
    #define DEVICE_HAS_BATTERY

    #undef STATUS_LED_ON
    #undef STATUS_LED_OFF
    #define STATUS_LED_ON() M5StickCLedSet(M5STICKC_LED_ON)
    #define STATUS_LED_OFF() M5StickCLedSet(M5STICKC_LED_OFF)

    #undef DISPLAY_PRINT
    #define DISPLAY_PRINT(str, x, y) TFT_print(str, x, y)
    #undef DISPLAY_WIDTH
    #define DISPLAY_WIDTH M5STICKC_DISPLAY_WIDTH
    #undef DISPLAY_HEIGHT
    #define DISPLAY_HEIGHT M5STICKC_DISPLAY_HEIGHT

    #define DEVICE_HAS_MAIN_BUTTON
    #define BUTTON_MAIN_EVENT_BASE M5STICKC_BUTTON_A_EVENT_BASE
    #define DEVICE_HAS_RESET_BUTTON
    #define BUTTON_RESET_EVENT_BASE M5STICKC_BUTTON_B_EVENT_BASE

#else

#endif

esp_err_t eDeviceInit(void);
esp_err_t eDeviceRegisterButtonCallback(esp_event_base_t base, void (*callback)(void * handler_arg, esp_event_base_t base, int32_t id, void * event_data) );

#endif /* ifndef _DEVICE_H_ */