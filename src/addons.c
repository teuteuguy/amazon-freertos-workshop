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

#define ADDONS_IO_CHECK_HANDLER( handler ) if ( handler == NULL ) { ESP_LOGE(TAG, "IO Handler is NULL"); return ADDONS_FAIL; }

#include "bmp280.h"

/*-----------------------------------------------------------*/

addons_err_t eAddonBmp280Init( IotI2CHandle_t const handle )
{
    ADDONS_IO_CHECK_HANDLER( handle );
    ESP_LOGD( TAG, "eAddonBmp280Init: Init" );
    if ( eBmp280Init( handle ) != BMP280_SUCCESS )
    {
        return ADDONS_FAIL;
    }
    return ADDONS_SUCCESS;
}

/*-----------------------------------------------------------*/

addons_err_t eAddonBmp280GetSensors( addon_bmp280_sensors_t * sensors, float current_altitude, float base_pressure )
{
    if ( eBmp280GetTemperatureAndPressure( &(sensors->temperature), &(sensors->pressure) ) != BMP280_SUCCESS )
    {
        return ADDONS_FAIL;
    }
    
    sensors->seaLevel = fBmp280GetSeaLevel( sensors->pressure, current_altitude );
    sensors->altitude = fBmp280GetAltitude( sensors->pressure, base_pressure );

    ESP_LOGD(TAG, "eAddonBmp280GetSensors: Temperature(%f)  Pressure(%f) SeaLevel(%f) Altitude(%f)", 
            sensors->temperature,
            sensors->pressure,
            sensors->seaLevel,
            sensors->altitude);

    return ADDONS_SUCCESS;
}

/*-----------------------------------------------------------*/

#include "mpu6886.h"

/*-----------------------------------------------------------*/

addons_err_t eAddonMPU6886Init( IotI2CHandle_t const handle )
{
    ADDONS_IO_CHECK_HANDLER( handle );
    ESP_LOGD( TAG, "eAddonMPU6886Init: Init" );
    if ( eMPU6886Init( handle ) != MPU6886_SUCCESS )
    {
        return ADDONS_FAIL;
    }
    return ADDONS_SUCCESS;
}

addons_err_t eAddonMPU6886GetSensors( addon_mpu6886_sensors_t * sensors )
{
    if ( eGetMPU6886AccelData( &(sensors->accel_x), &(sensors->accel_y), &(sensors->accel_z) ) != MPU6886_SUCCESS )
    {
        return ADDONS_FAIL;
    }
    if ( eGetMPU6886GyroData( &(sensors->gyro_x), &(sensors->gyro_y), &(sensors->gyro_z) ) != MPU6886_SUCCESS )
    {
        return ADDONS_FAIL;
    }
    if ( eGetMPU6886TempData( &(sensors->temperature) ) != MPU6886_SUCCESS )
    {
        return ADDONS_FAIL;
    }
    if ( eGetMPU6886AhrsData( &(sensors->pitch), &(sensors->roll), &(sensors->yaw) ) != MPU6886_SUCCESS )
    {
        return ADDONS_FAIL;
    }

    ESP_LOGD( TAG, "eAddonMPU6886GetSensors: Accel(%f, %f, %f)  Gyro(%f, %f, %f) Temp(%f) AHRS(%f, %f, %f)", 
                sensors->accel_x, sensors->accel_y, sensors->accel_z,
                sensors->gyro_x, sensors->gyro_y, sensors->gyro_z,
                sensors->temperature,
                sensors->pitch, sensors->roll, sensors->yaw );


    return ADDONS_SUCCESS;
}

/*-----------------------------------------------------------*/
