/**
 * @file lab2_shadow.h
 *
 * (C) 2019 - Timothee Cruse <timothee.cruse@gmail.com>
 * This code is licensed under the MIT License.
 */

#ifndef _LAB2_SHADOW_H_
#define _LAB2_SHADOW_H_

void lab2_init(const char *const strID);

#if defined(LAB_INIT)
    #undef LAB_INIT
    #define LAB_INIT lab2_init
#endif



#endif /* ifndef _LAB2_SHADOW_H_ */
