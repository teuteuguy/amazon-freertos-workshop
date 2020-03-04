/**
 * @file lab_config.h
 *
 * (C) 2019 - Timothee Cruse <timothee.cruse@gmail.com>
 * This code is licensed under the MIT License.
 */

#ifndef _LAB_CONFIG_H_
#define _LAB_CONFIG_H_

/* First step, choose the device you plan on running the labs on.
 *
 *          DEVICE_ESP32_DEVKITC
 *          DEVICE_M5STICKC
 * 
 * These defines will be used throughout the workshop code. */

#define DEVICE_ESP32_DEVKITC

/* To run a particular lab you need to define one of these.
 * Only one lab can be configured at a time
 *
 *          LABCONFIG_LAB0_DO_NOTHING
 *          LABCONFIG_LAB1_AWS_IOT_BUTTON
 *          LABCONFIG_LAB2_SHADOW
 *
 * These defines will be used throughout the workshop code. */

#define LABCONFIG_LAB1_AWS_IOT_BUTTON

#include "device.h"

#endif /* ifndef _LAB_CONFIG_H_ */
