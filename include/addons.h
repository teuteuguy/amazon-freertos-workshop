/**
 * @file addons.h
 *
 * (C) 2019 - Timothee Cruse <timothee.cruse@gmail.com>
 * This code is licensed under the MIT License.
 */

#ifndef _ADDONS_H_
#define _ADDONS_H_

#include "esp_err.h"

#include "lab_config.h"

#if defined( ADDON_BMP280 )

typedef struct {
    double temperature;
    double pressure;
    double sealevel;
    double altitude;

    double rawTemperature;
    double rawPressure;
} addon_bmp280_sensors_t;

esp_err_t eAddonBmp280Init( void );
esp_err_t eAddonBmp280GetSensors( addon_bmp280_sensors_t * sensors );

#endif /* defined( ADDON_BMP280 ) */

#if defined( ADDON_MPU6886 )

esp_err_t eAddonMPU6886Init( void );

#endif /* defined( ADDON_MPU6886 ) */

#endif /* ifndef _ADDONS_H_ */
