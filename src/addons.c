/**
 * @file addons.c
 * @brief Addon for the BMP280 sensor.
 *
 * (C) 2019 - Timothee Cruse <timothee.cruse@gmail.com>
 * This code is licensed under the MIT License.
 */

#include "addons.h"

#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "addons";

#if defined( ADDON_BMP280 )

#include "bmp280.h"

/*-----------------------------------------------------------*/

esp_err_t eAddonBmp280Init( void )
{
    esp_err_t res = ESP_FAIL;
    ESP_LOGI(TAG, "eAddonBmp280Init: Init");

    return res;
}

/*-----------------------------------------------------------*/

esp_err_t eAddonBmp280GetSensors( addon_bmp280_sensors_t * sensors )
{
    esp_err_t res = ESP_FAIL;

    return res;
}

/*-----------------------------------------------------------*/

#endif // ADDON_BMP280


#if defined( ADDON_MPU6886 )

#include "mpu6886.h"

/*-----------------------------------------------------------*/

esp_err_t eAddonMPU6886Init( void )
{
    esp_err_t res = ESP_FAIL;
    ESP_LOGI( TAG, "eAddonMPU6886Init: Init" );

    res = eMPU6886Init( NULL );

    return res;
}

/*-----------------------------------------------------------*/

#endif // ADDON_MPU6886