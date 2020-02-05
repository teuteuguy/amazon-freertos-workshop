/**
 * @file workshop.h
 *
 * (C) 2019 - Timothee Cruse <timothee.cruse@gmail.com>
 * This code is licensed under the MIT License.
 */

#ifndef _WORKSHOP__H_
#define _WORKSHOP__H_

#include "esp_err.h"

/* For simplicity of setting up WIFI and other libraries, 
 * we re-use the Amazon FreeRTOS demos. Here we re-define to run specific Workshop Labs */
#define DEMO_RUNNER_RunDemos workshop_run

esp_err_t workshop_run(void);

#endif /* ifndef _WORKSHOP__H_ */
