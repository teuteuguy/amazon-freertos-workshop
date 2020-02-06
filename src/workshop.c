/**
 * @file workshop.c
 * @brief Workshop code to run different labs.
 *
 * (C) 2019 - Timothee Cruse <timothee.cruse@gmail.com>
 * This code is licensed under the MIT License.
 */

/* The config header is always included first. */
#include "iot_config.h"

/* Standard includes. */
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* Set up logging for this demo. */
#include "iot_demo_logging.h"

/* Platform layer includes. */
#include "platform/iot_clock.h"
#include "platform/iot_threads.h"

#include "aws_demo.h"
#include "types/iot_network_types.h"
#include "semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "m5stickc.h"

#include "lab_config.h"
#include "workshop.h"

/* Declaration of demo functions. */
#if defined(LABCONFIG_LAB1_AWS_IOT_BUTTON) || defined(LABCONFIG_LAB2_SHADOW)
#include "lab1_aws_iot_button.h"
#endif // LABCONFIG_LAB1_AWS_IOT_BUTTON || LABCONFIG_LAB2_SHADOW
#ifdef LABCONFIG_LAB2_SHADOW
#include "lab2_shadow.h"
#endif // LABCONFIG_LAB2_SHADOW

#include "lab_connection.h"

/*-----------------------------------------------------------*/

static const char *TAG = "workshop";

/*-----------------------------------------------------------*/

uint8_t uMACAddr[6] = { 0 };
#define MAC_ADDRESS_STR_LENGTH ( sizeof(uMACAddr) * 2 + 1 )
char strMACAddr[MAC_ADDRESS_STR_LENGTH] = "";

/*-----------------------------------------------------------*/

esp_err_t draw_battery_level(void);
void battery_refresh_timer_init(void);

void accel_timer_init(void);

esp_err_t workshop_init(void);

#if defined(LABCONFIG_LAB1_AWS_IOT_BUTTON) || defined(LABCONFIG_LAB2_SHADOW)

IotSemaphore_t lab1_semaphore;

#endif // LABCONFIG_LAB1_AWS_IOT_BUTTON || LABCONFIG_LAB2_SHADOW


/*-----------------------------------------------------------*/

esp_err_t workshop_run(void)
{
    esp_err_t res = esp_efuse_mac_get_default(uMACAddr);

    if (res == ESP_OK)
    {
        int status = snprintf( strMACAddr, MAC_ADDRESS_STR_LENGTH, "%02x%02x%02x%02x%02x%02x",
                    uMACAddr[0], uMACAddr[1], uMACAddr[2], uMACAddr[3], uMACAddr[4], uMACAddr[5] );
        if( status < 0 )
        {
            ESP_LOGE(TAG, "Error generating the ID: %d", (int) status);
            return ESP_FAIL;
        }
    }

    ESP_LOGI(TAG, "Device MAC Address: %s", strMACAddr);

    if (res == ESP_OK)
    {
        res = workshop_init();
    }

    return res;
}

/*-----------------------------------------------------------*/

void button_event_handler(void * handler_arg, esp_event_base_t base, int32_t id, void * event_data)
{
    if (base == M5STICKC_BUTTON_A_EVENT_BASE )
    {

        if ( id == M5STICKC_BUTTON_CLICK_EVENT )
        {
            ESP_LOGI(TAG, "Button A Pressed");            
        }
        if ( id == M5STICKC_BUTTON_HOLD_EVENT )
        {
            ESP_LOGI(TAG, "Button A Held");            
        }
        
#if defined(LABCONFIG_LAB1_AWS_IOT_BUTTON) || defined(LABCONFIG_LAB2_SHADOW)
        IotSemaphore_Wait(&lab1_semaphore);
        lab1_action(strMACAddr, id);
        IotSemaphore_Post(&lab1_semaphore);
#endif // LABCONFIG_LAB1_AWS_IOT_BUTTON || LABCONFIG_LAB2_SHADOW

    }

    if (base == M5STICKC_BUTTON_B_EVENT_BASE && id == M5STICKC_BUTTON_HOLD_EVENT) {
        ESP_LOGI(TAG, "Button B Held");
        ESP_LOGI(TAG, "Restarting");
        esp_restart();
    }
}

esp_err_t workshop_init(void)
{
    esp_err_t res = ESP_FAIL;

    ESP_LOGI(TAG, "======================================================");
    ESP_LOGI(TAG, "workshop_init: ...");

    m5stickc_config_t m5stickc_config;
    m5stickc_config.power.enable_lcd_backlight = false;
    m5stickc_config.power.lcd_backlight_level = 1;

    res = M5StickCInit(&m5stickc_config);
    ESP_LOGI(TAG, "workshop_init: M5StickCInit ...       %s", res == ESP_OK ? "OK" : "NOK");
    if (res != ESP_OK) return res;

    accel_timer_init();

    TFT_FONT_ROTATE = 0;
    TFT_TEXT_WRAP = 0;
    TFT_FONT_TRANSPARENT = 0;
    TFT_FONT_FORCEFIXED = 0;
    TFT_GRAY_SCALE = 0;
    TFT_setGammaCurve(DEFAULT_GAMMA_CURVE);
    TFT_setRotation(LANDSCAPE_FLIP);
    TFT_setFont(DEFAULT_FONT, NULL);
    TFT_resetclipwin();
    TFT_fillScreen(TFT_BLACK);
    TFT_FONT_BACKGROUND = TFT_BLACK;
    TFT_FONT_FOREGROUND = TFT_ORANGE;
    res = M5StickCDisplayOn();
    ESP_LOGI(TAG, "              LCD Backlight ON ...    %s", res == ESP_OK ? "OK" : "NOK");
    if (res != ESP_OK) return res;

    #define SCREEN_OFFSET 2
    #define SCREEN_LINE_HEIGHT 14
    #define SCREEN_LINE_1  SCREEN_OFFSET + 0 * SCREEN_LINE_HEIGHT
    #define SCREEN_LINE_2  SCREEN_OFFSET + 1 * SCREEN_LINE_HEIGHT
    #define SCREEN_LINE_3  SCREEN_OFFSET + 2 * SCREEN_LINE_HEIGHT
    #define SCREEN_LINE_4  SCREEN_OFFSET + 3 * SCREEN_LINE_HEIGHT

    TFT_print((char *)"Amazon FreeRTOS", CENTER, SCREEN_LINE_1);
    TFT_print((char *)"workshop", CENTER, SCREEN_LINE_2);

#ifdef LABCONFIG_LAB0_SETUP
    TFT_print((char *)"LAB0 - SETUP", CENTER, SCREEN_LINE_4);
#endif // LABCONFIG_LAB0_SETUP

#ifdef LABCONFIG_LAB1_AWS_IOT_BUTTON
    TFT_print((char *)"LAB1 - AWS IOT BUTTON", CENTER, SCREEN_LINE_4);
#endif // LABCONFIG_LAB1_AWS_IOT_BUTTON

#ifdef LABCONFIG_LAB2_SHADOW
    TFT_print((char *)"LAB2 - THING SHADOW", CENTER, SCREEN_LINE_4);
    lab2_init(strMACAddr);
#endif // LABCONFIG_LAB2_SHADOW

    TFT_drawLine(0, M5STICKC_DISPLAY_HEIGHT - 13 - 3, M5STICKC_DISPLAY_WIDTH, M5STICKC_DISPLAY_HEIGHT - 13 - 3, TFT_ORANGE);
    
    res = draw_battery_level();
    battery_refresh_timer_init();

    res = esp_event_handler_register_with(m5stickc_event_event_loop, M5STICKC_BUTTON_A_EVENT_BASE, ESP_EVENT_ANY_ID, button_event_handler, NULL);
    ESP_LOGI(TAG, "              Button A registered ... %s", res == ESP_OK ? "OK" : "NOK");
    if (res != ESP_OK) return res;

    res = esp_event_handler_register_with(m5stickc_event_event_loop, M5STICKC_BUTTON_B_EVENT_BASE, ESP_EVENT_ANY_ID, button_event_handler, NULL);
    ESP_LOGI(TAG, "              Button B registered ... %s", res == ESP_OK ? "OK" : "NOK");
    if (res != ESP_OK) return res;

    ESP_LOGI(TAG, "workshop_init: ... done");
    ESP_LOGI(TAG, "======================================================");

#if defined(LABCONFIG_LAB1_AWS_IOT_BUTTON) || defined(LABCONFIG_LAB2_SHADOW)

    if (!IotSemaphore_Create(&lab1_semaphore, 0, 1))
    {
        ESP_LOGE(TAG, "Failed to create Lab 1 semaphore!");
        res = ESP_FAIL;
    }

    if (res == ESP_OK)
    {
        /* Init the Semaphore to release it */
        IotSemaphore_Post(&lab1_semaphore);
    }

#endif // LABCONFIG_LAB1_AWS_IOT_BUTTON || LABCONFIG_LAB2_SHADOW

#ifdef LABCONFIG_LAB1_AWS_IOT_BUTTON

    lab1_init(strMACAddr);

    // Create semaphore for lab1
    if ( res == ESP_OK )
    {
        /* Take the Semaphore */
        IotSemaphore_Wait( &lab1_semaphore );
        
        if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0)
        {
            // Woken up by our button
            ESP_LOGI( TAG, "                    Woken up by the button" );
            lab1_action(strMACAddr, M5STICKC_BUTTON_CLICK_EVENT);
        }
        else
        {
            // Woken up by other
            ESP_LOGI( TAG, "                    Woken up by other!" );
        }

        IotSemaphore_Post( &lab1_semaphore );
    }

#endif // LABCONFIG_LAB1_AWS_IOT_BUTTON

    return res;
}

/*-----------------------------------------------------------*/

static const TickType_t xBatteryRefreshTimerFrequency_ms = 10000UL;
static TimerHandle_t xBatteryRefresh;

esp_err_t draw_battery_level(void)
{
    esp_err_t res = ESP_FAIL;
    int status = EXIT_SUCCESS;
    uint16_t vbat = 0, vaps = 0, b, c, battery;
    char pVbatStr[11] = {0};

    res = M5StickCPowerGetVbat(&vbat);
    res |= M5StickCPowerGetVaps(&vaps);

    if (res == ESP_OK)
    {
        ESP_LOGD(TAG, "draw_battery_level: VBat:         %u", vbat);
        ESP_LOGD(TAG, "draw_battery_level: VAps:         %u", vaps);
        b = (vbat * 1.1);
        ESP_LOGD(TAG, "draw_battery_level: b:            %u", b);
        c = (vaps * 1.4);
        ESP_LOGD(TAG, "draw_battery_level: c:            %u", c);
        battery = ((b - 3000)) / 12;
        ESP_LOGD(TAG, "draw_battery_level: battery:      %u", battery);

        if (battery >= 100)
        {
            battery = 99; // No need to support 100% :)
        }

        if (c >= 4500) //4.5)
        {
            status = snprintf(pVbatStr, 11, "CHG: %02u%%", battery);
        }
        else
        {
            status = snprintf(pVbatStr, 11, "BAT: %02u%%", battery);
        }

        if (status < 0) {
            ESP_LOGE(TAG, "draw_battery_level: error with creating battery string");
        }
        else
        {
            ESP_LOGD(TAG, "draw_battery_level: Charging str(%i): %s", status, pVbatStr);
            TFT_print(pVbatStr, 1, M5STICKC_DISPLAY_HEIGHT - 13);
        }
    }

    return res;
}

static void prvBatteryRefreshTimerCallback(TimerHandle_t pxTimer)
{
    draw_battery_level();    
}

void battery_refresh_timer_init(void)
{
    xBatteryRefresh = xTimerCreate("BatteryRefresh", pdMS_TO_TICKS(xBatteryRefreshTimerFrequency_ms), pdTRUE, NULL, prvBatteryRefreshTimerCallback);
    xTimerStart(xBatteryRefresh, 0);
}

/*-----------------------------------------------------------*/

static const TickType_t xAccelTimerFrequency_ms = 1000UL;
static TimerHandle_t xAccelTimer;

static void prvAccelTimerCallback(TimerHandle_t pxTimer)
{
    float ax, ay, az, gx, gy, gz, t;
    esp_err_t e;
    e = M5StickCMPU6886GetAccelData( &ax, &ay, &az );
    if (e != ESP_OK)
    {
        return;
    }
    e = M5StickCMPU6886GetGyroData( &gx, &gy, &gz );
    if (e != ESP_OK)
    {
        return;
    }
    e = M5StickCMPU6886GetTempData( &t );
    if (e != ESP_OK)
    {
        return;
    }

    ESP_LOGI(TAG, "MPU6886: %f, %f, %f, %f, %f, %f, %f", ax, ay, az, gx, gy, gz, t);
}

void accel_timer_init(void)
{
    xAccelTimer = xTimerCreate("xAccelTimer", pdMS_TO_TICKS(xAccelTimerFrequency_ms), pdTRUE, NULL, prvAccelTimerCallback);
    xTimerStart(xAccelTimer, 0);
}
