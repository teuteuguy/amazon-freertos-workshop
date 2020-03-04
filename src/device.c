/**
 * @file device.c
 * @brief Device specific code.
 *
 * (C) 2019 - Timothee Cruse <timothee.cruse@gmail.com>
 * This code is licensed under the MIT License.
 */

#include "esp_event.h"
#include "esp_log.h"

#include "device.h"

/*-----------------------------------------------------------*/

static const char *TAG = "device";

/*-----------------------------------------------------------*/

#if defined(DEVICE_ESP32_DEVKITC)

#elif defined(DEVICE_M5STICKC)

    static TaskHandle_t xAccelerometerTaskHandle;
    static void prvAccelerometerTask( void *pvParameters );

    static TaskHandle_t xBatteryTaskHandle;
    static void prvBatteryTask( void *pvParameters );

#endif


/*-----------------------------------------------------------*/
/*----                   Display                         ----*/
/*-----------------------------------------------------------*/

#if defined(DEVICE_M5STICKC)

    esp_err_t display_init(void)
    {
        esp_err_t res = ESP_FAIL;

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

        if (res == ESP_OK)
        {
            #define SCREEN_OFFSET 2
            #define SCREEN_LINE_HEIGHT 14
            #define SCREEN_LINE_1  SCREEN_OFFSET + 0 * SCREEN_LINE_HEIGHT
            #define SCREEN_LINE_2  SCREEN_OFFSET + 1 * SCREEN_LINE_HEIGHT
            #define SCREEN_LINE_3  SCREEN_OFFSET + 2 * SCREEN_LINE_HEIGHT
            #define SCREEN_LINE_4  SCREEN_OFFSET + 3 * SCREEN_LINE_HEIGHT

            TFT_print((char *)"Amazon FreeRTOS", CENTER, SCREEN_LINE_1);
            TFT_print((char *)"workshop", CENTER, SCREEN_LINE_2);

            #if defined(LABCONFIG_LAB0_DO_NOTHING)
                TFT_print((char *)"LAB0 - DOES NOTHING", CENTER, SCREEN_LINE_4);
            #elif defined(LABCONFIG_LAB1_AWS_IOT_BUTTON)
                TFT_print((char *)"LAB1 - AWS IOT BUTTON", CENTER, SCREEN_LINE_4);
            #elif defined(LABCONFIG_LAB2_SHADOW)
                TFT_print((char *)"LAB2 - THING SHADOW", CENTER, SCREEN_LINE_4);
            #endif

            TFT_drawLine(0, M5STICKC_DISPLAY_HEIGHT - 13 - 3, M5STICKC_DISPLAY_WIDTH, M5STICKC_DISPLAY_HEIGHT - 13 - 3, TFT_ORANGE);
        }

        return res;
    }

#endif

/*-----------------------------------------------------------*/

esp_err_t eDeviceInit(void)
{
    esp_err_t res = ESP_FAIL;

    #if defined(DEVICE_ESP32_DEVKITC)

        res = eESP32DevkitcInit();
        ESP_LOGI(TAG, "eDeviceInit: ESP32 DevkitC Init ... %s", res == ESP_OK ? "OK" : "NOK");
        if (res != ESP_OK) return res;

    #elif defined(DEVICE_M5STICKC)
        m5stickc_config_t m5stickc_config;
        m5stickc_config.power.enable_lcd_backlight = false;
        m5stickc_config.power.lcd_backlight_level = 1;

        res = M5StickCInit(&m5stickc_config);
        ESP_LOGI(TAG, "eDeviceInit: M5StickC Init ...      %s", res == ESP_OK ? "OK" : "NOK");
        if (res != ESP_OK) return res;

        res = display_init();
        ESP_LOGI(TAG, "eDeviceInit: LCD Backlight ON ...   %s", res == ESP_OK ? "OK" : "NOK");
        if (res != ESP_OK) return res;

    #endif // device type

    #if defined(DEVICE_HAS_ACCELEROMETER)

        /* Create Accelerometer reading task. */
        xTaskCreate( prvAccelerometerTask,			/* The function that implements the task. */
                    "AccelTask",    				/* The text name assigned to the task - for debug only as it is not used by the kernel. */
                    2048,		/* The size of the stack to allocate to the task. */
                    NULL,                           /* The parameter passed to the task - in this case the counter to increment. */
                    0,				                /* The priority assigned to the task. */
                    &xAccelerometerTaskHandle );	/* The task handle is used to obtain the name of the task. */
        // ESP_LOGI(TAG, "eDeviceInit: Accelerometer task init... %s", res == ESP_OK ? "OK" : "NOK");

    #endif // defined(DEVICE_HAS_ACCELEROMETER)

    #if defined(DEVICE_HAS_BATTERY)

        /* Create Battery reading task. */
        xTaskCreate( prvBatteryTask,			    /* The function that implements the task. */
                    "BatteryTask",    				/* The text name assigned to the task - for debug only as it is not used by the kernel. */
                    2048,		                    /* The size of the stack to allocate to the task. */
                    NULL,                           /* The parameter passed to the task - in this case the counter to increment. */
                    0,				                /* The priority assigned to the task. */
                    &xBatteryTaskHandle );	        /* The task handle is used to obtain the name of the task. */

    #endif // defined(DEVICE_HAS_BATTERY)

    return res;
}

/*-----------------------------------------------------------*/

#if defined(DEVICE_ESP32_DEVKITC)
    #define DEVICE_BUTTON_EVENT_LOOP esp32devkitc_event_loop
#elif defined(DEVICE_M5STICKC)
    #define DEVICE_BUTTON_EVENT_LOOP m5stickc_event_loop
#else
    esp_event_loop_handle_t dummy_event_event_loop;
    #define DEVICE_BUTTON_EVENT_LOOP dummy_event_event_loop
#endif

esp_err_t eDeviceRegisterButtonCallback(esp_event_base_t base, void (*callback)(void * handler_arg, esp_event_base_t base, int32_t id, void * event_data) )
{
    esp_err_t res = esp_event_handler_register_with(DEVICE_BUTTON_EVENT_LOOP, base, ESP_EVENT_ANY_ID, callback, NULL);
    ESP_LOGD(TAG, "eDeviceRegisterButtonCallback: Button registered... %s", res == ESP_OK ? "OK" : "NOK");
    return res;
}

/*-----------------------------------------------------------*/

#if defined(DEVICE_HAS_ACCELEROMETER)
    static void prvAccelerometerTask( void *pvParameters )
    {
        TickType_t xDelayTimeInTicks = pdMS_TO_TICKS( 1000 );

        for( ;; )
        {
            #if defined(DEVICE_M5STICKC)
                float ax, ay, az, gx, gy, gz, t, pitch, roll, yaw;
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
                e = M5StickCMPU6886GetAhrsData( &pitch, &roll, &yaw );
                if (e != ESP_OK)
                {
                    return;
                }

                ESP_LOGD(TAG, "MPU6886: Accel(%f, %f, %f)  Gyro(%f, %f, %f) Temp(%f) AHRS(%f, %f, %f)", ax, ay, az, gx, gy, gz, t, pitch, roll, yaw);
            #endif

            vTaskDelay( xDelayTimeInTicks );
        }

        vTaskDelete( NULL );
    }
#endif // defined(DEVICE_HAS_ACCELEROMETER)

/*-----------------------------------------------------------*/

#if defined(DEVICE_HAS_BATTERY)
    static void prvBatteryTask( void *pvParameters )
    {
        TickType_t xDelayTimeInTicks = pdMS_TO_TICKS( 10000 );

        for( ;; )
        {

            #if defined(DEVICE_M5STICKC)

                esp_err_t res = ESP_FAIL;
                int status = EXIT_SUCCESS;
                uint16_t vbat = 0, vaps = 0, b = 0, c = 0, battery = 0;
                char pVbatStr[11] = {0};

                res = M5StickCPowerGetVbat(&vbat);
                res |= M5StickCPowerGetVaps(&vaps);

                if (res == ESP_OK)
                {
                    ESP_LOGD(TAG, "prvBatteryTask: VBat:         %u", vbat);
                    ESP_LOGD(TAG, "prvBatteryTask: VAps:         %u", vaps);
                    b = (vbat * 1.1);
                    ESP_LOGD(TAG, "prvBatteryTask: b:            %u", b);
                    c = (vaps * 1.4);
                    ESP_LOGD(TAG, "prvBatteryTask: c:            %u", c);
                    battery = ((b - 3000)) / 12;
                    ESP_LOGD(TAG, "prvBatteryTask: battery:      %u", battery);

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
                        ESP_LOGE(TAG, "prvBatteryTask: error with creating battery string");
                    }
                    else
                    {
                        ESP_LOGD(TAG, "prvBatteryTask: Charging str(%i): %s", status, pVbatStr);
                        TFT_print(pVbatStr, 1, M5STICKC_DISPLAY_HEIGHT - 13);
                    }
                }

            #endif

            vTaskDelay( xDelayTimeInTicks );
        }

        vTaskDelete( NULL );
    }
#endif

/*-----------------------------------------------------------*/
