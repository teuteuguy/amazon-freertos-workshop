/**
 * @file addons.h
 *
 * (C) 2019 - Timothee Cruse <timothee.cruse@gmail.com>
 * This code is licensed under the MIT License.
 */

#ifndef _ADDONS_H_
#define _ADDONS_H_

#include "iot_i2c.h"
#include "lab_config.h"

typedef int32_t addons_err_t;

/**
 * @brief   ADDONS FAILURE CODES
 */
#define ADDONS_SUCCESS             ( 0 )     /** Operation completed successfully. */
#define ADDONS_FAIL                ( 1 )     /** Operation failed. */
#define ADDONS_OTHER_ERROR         ( 2 )     /** Other error. */

typedef struct {
    float temperature;
    float pressure;
    float seaLevel;
    float altitude;
} addon_bmp280_sensors_t;

addons_err_t eAddonBmp280Init( IotI2CHandle_t const handle );
addons_err_t eAddonBmp280GetSensors( addon_bmp280_sensors_t * sensors, float current_altitude, float base_pressure );

typedef struct {
    float accel_x;
    float accel_y;
    float accel_z;
    float gyro_x;
    float gyro_y;
    float gyro_z;
    float temperature;
    float pitch;
    float roll;
    float yaw;
} addon_mpu6886_sensors_t;

addons_err_t eAddonMPU6886Init( IotI2CHandle_t const handle );
addons_err_t eAddonMPU6886GetSensors( addon_mpu6886_sensors_t * sensors );

#endif /* ifndef _ADDONS_H_ */
