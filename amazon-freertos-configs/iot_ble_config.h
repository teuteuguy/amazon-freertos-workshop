/**
 * @file iot_ble_config.h
 * @brief BLE configuration overrides for ESP32 board.
 * 
 * (C) 2019 - Timothee Cruse <timothee.cruse@gmail.com>
 * This code is licensed under the MIT License.
 */

#ifndef _IOT_BLE_CONFIG_H_
#define _IOT_BLE_CONFIG_H_

#include "lab_config.h"

#if defined(LABCONFIG_WIFI_PROVISION_VIA_BLE)

/* Device name for this peripheral device. */
#define IOT_BLE_DEVICE_COMPLETE_LOCAL_NAME      "My Awesome Device"

/* Enable WIFI provisioning GATT service. */
#define IOT_BLE_ENABLE_WIFI_PROVISIONING         ( 1 )
#define IOT_BLE_ENABLE_GATT_DEMO                 ( 0 )

/* Disable numeric comparison */
#define IOT_BLE_ENABLE_NUMERIC_COMPARISON        ( 0 )
#define IOT_BLE_ENABLE_SECURE_CONNECTION         ( 0 )
#define IOT_BLE_INPUT_OUTPUT                     ( eBTIONone )

#define IOT_BLE_ENCRYPTION_REQUIRED               ( 0 )

#endif

/* Include BLE default config at bottom to set the default values for the configurations which are not overridden */
#include "iot_ble_config_defaults.h"

#endif /* _IOT_BLE_CONFIG_H_ */
