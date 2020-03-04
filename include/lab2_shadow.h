/**
 * @file lab2_shadow.h
 *
 * (C) 2019 - Timothee Cruse <timothee.cruse@gmail.com>
 * This code is licensed under the MIT License.
 */

#ifndef _LAB2_SHADOW_H_
#define _LAB2_SHADOW_H_

#include "esp_err.h"

esp_err_t eLab2Init(const char *const strID);

#if defined(LAB_INIT)
    #undef LAB_INIT
    #define LAB_INIT(x) eLab2Init(x)
#endif

#endif /* ifndef _LAB2_SHADOW_H_ */
