/**
 * @file lab_config.h
 *
 * (C) 2019 - Timothee Cruse <timothee.cruse@gmail.com>
 * This code is licensed under the MIT License.
 */

#ifndef _LAB_CONFIG_H_
#define _LAB_CONFIG_H_

#include "stdint.h"

/* To run a particular lab you need to define one of these.
 * Only one lab can be configured at a time
 *
 *          LABCONFIG_LAB0_SETUP
 *          LABCONFIG_LAB1_AWS_IOT_BUTTON
 *          LABCONFIG_LAB2_SHADOW
 *
 *  These defines are used in iot_demo_runner.h for demo selection */

#define LABCONFIG_LAB0_SETUP

#endif /* ifndef _LAB_CONFIG_H_ */
