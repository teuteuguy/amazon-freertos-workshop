/**
 * m5_display.h
 *
 * (C) 2020 - Timothee Cruse <timothee.cruse@gmail.com>
 * This code is licensed under the MIT License.
 */

#ifndef _M5_DISPLAY_H_
#define _M5_DISPLAY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_system.h"
#include "esp_log.h"
#include "util/spi_master_lobo.h"
#include "util/tftspi.h"
#include "util/tft.h"

#define M5STICKC_DISPLAY_TYPE   DISP_TYPE_ST7735S       /*!< Display type for display driver */
#define M5STICKC_DISPLAY_WIDTH  160                     /*!< Display width in pixels after rotation */
#define M5STICKC_DISPLAY_HEIGHT 80                      /*!< Display height in pixels after rotation */

// Defines for global variables of the TFT Library
#define TFT_ORIENTATION         orientation
#define TFT_FONT_ROTATE         font_rotate
#define TFT_TEXT_WRAP           text_wrap
#define TFT_FONT_TRANSPARENT    font_transparent
#define TFT_FONT_FORCEFIXED     font_forceFixed
#define TFT_GRAY_SCALE          gray_scale
#define TFT_FONT_BACKGROUND     _bg
#define TFT_FONT_FOREGROUND     _fg

extern spi_lobo_device_handle_t m5_display_spi;   /*!< SPI device handle */

/**
 * @brief   Initialize display
 *
 * @return  ESP_OK success
 *          ESP_FAIL failed
 */
esp_err_t eM5DisplayInit( void );

esp_err_t eM5DisplayPrint( char * str, int x, int y );
esp_err_t eM5DisplayDrawLine( int x1, int y1, int x2, int y2, color_t color );

// /**
//  * @brief   Set display backlight level
//  *
//  * @param   backlight_level Backlight level from 0 (lowest) to 7 (brightest)
//  *
//  * @return  ESP_OK success
//  *          ESP_FAIL failed
//  */
// esp_err_t M5StickCDisplaySetBacklightLevel(uint8_t backlight_level);

// /**
//  * @brief   Sets a timeout to turn display off
//  *
//  *          Display turns back on with button events or M5StickCDisplayWakeup() function call.
//  *
//  * @param   timeout timeout in seconds
//  * @return  ESP_OK success
//  *          ESP_FAIL failed
//  */
// esp_err_t M5StickCDisplayTimeout(uint32_t timeout);

// /**
//  * @brief   Turns display on and resets timeout timer
//  */
// void M5StickCDisplayWakeup();

// /**
//  * @brief   Callback for timeout to turn display off
//  */
// void M5StickCDisplaySleep();

// /**
//  * @brief   Event handler for display. Not meant to be used by user program.
//  */
// void M5StickCDisplayEventHandler(void * handler_arg, esp_event_base_t base, int32_t id, void * event_data);

#ifdef __cplusplus
}
#endif

#endif // _M5_DISPLAY_H_