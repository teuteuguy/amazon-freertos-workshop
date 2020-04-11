/**
 * @file lab_config.h
 *
 * (C) 2019 - Timothee Cruse <timothee.cruse@gmail.com>
 * This code is licensed under the MIT License.
 */

#ifndef _LAB_CONFIG_H_
#define _LAB_CONFIG_H_

#include "driver/i2c.h"

/* First step, choose the device you plan on running the labs on.
 *
 *          DEVICE_ESP32_DEVKITC
 *          DEVICE_M5STICKC
 * 
 * These defines will be used throughout the workshop code. */

#define DEVICE_M5STICKC

/* To run a particular lab you need to define one of these.
 * Only one lab can be configured at a time
 *
 *          LABCONFIG_LAB0_DO_NOTHING
 *          LABCONFIG_LAB1_AWS_IOT_BUTTON
 *          LABCONFIG_LAB2_SHADOW
 *
 * These defines will be used throughout the workshop code. */

#define LABCONFIG_LAB0_DO_NOTHING


/* If you want to allow WIFI provisioning to be managed by mobile apps.
 * Uncomment following #define.
 * Note: the wifi provisioning apps on Android or iOS are required.*/

// #define LABCONFIG_WIFI_PROVISION_VIA_BLE



/* Uncomment some of the following if you extra capabilities based on
 * extra shields and addons.
 * 
 *          ADDON_BMP280 - Adds BMP280 capabilities
 * 
 * */

// #define ADDON_BMP280     I2C_NUM_1  // On IRC port 1

#endif /* ifndef _LAB_CONFIG_H_ */
