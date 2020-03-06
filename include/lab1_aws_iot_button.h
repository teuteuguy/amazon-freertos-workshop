/**
 * @file lab1_aws_iot_button.h
 *
 * (C) 2019 - Timothee Cruse <timothee.cruse@gmail.com>
 * This code is licensed under the MIT License.
 */

#ifndef _LAB1_AWS_IOT_BUTTON_H_
#define _LAB1_AWS_IOT_BUTTON_H_

#include "esp_err.h"

#if defined(LAB_INIT)
    #undef LAB_INIT
    #define LAB_INIT(x) eLab1Init(x)
#endif

esp_err_t eLab1Init(const char *strID);
esp_err_t eLab1Action( const char * strID, int32_t buttonID );

#endif /* ifndef _LAB1_AWS_IOT_BUTTON_H_ */
