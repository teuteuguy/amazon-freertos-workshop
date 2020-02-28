/**
 * @file lab1_aws_iot_button.h
 *
 * (C) 2019 - Timothee Cruse <timothee.cruse@gmail.com>
 * This code is licensed under the MIT License.
 */

#ifndef _LAB1_AWS_IOT_BUTTON_H_
#define _LAB1_AWS_IOT_BUTTON_H_

void lab1_init(const char *strID);
void lab1_action(const char *strID, int32_t buttonID);

#if defined(LAB_INIT)
    #undef LAB_INIT
    #define LAB_INIT lab1_init
#endif

#define BUTTON_A_PRESS_ACTION lab1_action

#endif /* ifndef _LAB1_AWS_IOT_BUTTON_H_ */
