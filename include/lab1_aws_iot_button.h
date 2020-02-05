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

#endif /* ifndef _LAB1_AWS_IOT_BUTTON_H_ */
