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
#include "esp_event.h"

#include "device.h"
#include "lab_config.h"
#include "workshop.h"

/* Declaration of demo functions. */
#if defined(LABCONFIG_LAB1_AWS_IOT_BUTTON) || defined(LABCONFIG_LAB2_SHADOW)
    #include "lab1_aws_iot_button.h"
#endif
#if defined(LABCONFIG_LAB2_SHADOW)
    #include "lab2_shadow.h"
#endif

#include "lab_connection.h"

/*-----------------------------------------------------------*/

static const char *TAG = "workshop";

/*-----------------------------------------------------------*/

uint8_t uMACAddr[6] = { 0 };
#define MAC_ADDRESS_STR_LENGTH ( sizeof(uMACAddr) * 2 + 1 )
char strMACAddr[MAC_ADDRESS_STR_LENGTH] = "";

device_t prvDeviceConfig = {
    .i2c_handler = { NULL, NULL }
};

/*-----------------------------------------------------------*/

esp_err_t eWorkshopInit(void);

/*-----------------------------------------------------------*/

esp_err_t eWorkshopRun(void)
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
        res = eWorkshopInit();
    }

    return res;
}

/*-----------------------------------------------------------*/

#if defined(DEVICE_HAS_MAIN_BUTTON)
    void prvWorkshopMainButtonEventHandler(void * handler_arg, esp_event_base_t base, int32_t id, void * event_data)
    {
        if (base == BUTTON_MAIN_EVENT_BASE )
        {
            if ( id == BUTTON_CLICK )
            {
                ESP_LOGI(TAG, "Main Button Pressed");
            }
            if ( id == BUTTON_HOLD )
            {
                ESP_LOGI(TAG, "Main Button Held");
            }
        }

        #if defined(LABCONFIG_LAB1_AWS_IOT_BUTTON)|| defined(LABCONFIG_LAB2_SHADOW)
        if ( eLab1Action( strMACAddr, id ) != ESP_OK ) 
        {
            ESP_LOGE(TAG, "Failed to run Lab1 Action");
        }
        #endif
    }
#endif // defined(DEVICE_HAS_MAIN_BUTTON)

/*-----------------------------------------------------------*/

#if defined(DEVICE_HAS_RESET_BUTTON)
    void prvWorkshopResetButtonEventHandler(void * handler_arg, esp_event_base_t base, int32_t id, void * event_data)
    {
        if (base == BUTTON_RESET_EVENT_BASE) {

            if (id == BUTTON_HOLD) {
                ESP_LOGI(TAG, "Reset Button Held");
                ESP_LOGI(TAG, "Reseting Wifi Networks");
                vLabConnectionResetWifiNetworks();
            }
            if (id == BUTTON_CLICK) {
                ESP_LOGI(TAG, "Reset Button Clicked");
                ESP_LOGI(TAG, "Restarting in 2secs");
                vTaskDelay( pdMS_TO_TICKS( 2000 ) );
                esp_restart();
            }
        }
    }
#endif // defined(DEVICE_HAS_RESET_BUTTON)

/*-----------------------------------------------------------*/

esp_err_t eWorkshopInit(void)
{
    esp_err_t res = ESP_FAIL;

    ESP_LOGI(TAG, "eWorkshopInit: ... ===================================");

    if ( eDeviceInit( &prvDeviceConfig ) ==  DEVICE_SUCCESS )
    {
        
        #if defined(DEVICE_HAS_MAIN_BUTTON)
            res = eDeviceRegisterButtonCallback( BUTTON_MAIN_EVENT_BASE, prvWorkshopMainButtonEventHandler );
            if (res != ESP_OK)
            {
                ESP_LOGE( TAG, "eWorkshopInit: Register main button ... failed" );
            }
        #endif // defined(DEVICE_HAS_MAIN_BUTTON)

        #if defined(DEVICE_HAS_RESET_BUTTON)
            res = eDeviceRegisterButtonCallback( BUTTON_RESET_EVENT_BASE, prvWorkshopResetButtonEventHandler );
            if (res !=  ESP_OK)
            {
                ESP_LOGE( TAG, "eWorkshopInit: Register reset button ... failed" );
            }
        #endif // defined(DEVICE_HAS_RESET_BUTTON)

        /* Init the labs */
        if ( LAB_INIT( strMACAddr ) == ESP_OK )
        {
            ESP_LOGI(TAG, "eWorkshopInit: ... done");
            res = ESP_OK;            
        }
        else
        {
            ESP_LOGE(TAG, "eWorkshopInit: Init labs ... failed");
        }        
    }
    else
    {
        ESP_LOGE(TAG, "eWorkshopInit: eDeviceInit ... failed");
    }

    ESP_LOGI(TAG, "======================================================");

    return res;
}

/*-----------------------------------------------------------*/
