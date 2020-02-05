/**
 * @file lab_connection.h
 *
 * (C) 2019 - Timothee Cruse <timothee.cruse@gmail.com>
 * This code is licensed under the MIT License.
 */

#ifndef _LAB_CONNECTION_H_
#define _LAB_CONNECTION_H_

#include "esp_err.h"
#include "iot_mqtt.h"
#include "aws_iot_shadow.h"

typedef struct {
    char * strID;
    bool useShadow;
    networkConnectedCallback_t networkConnectedCallback;
    networkDisconnectedCallback_t networkDisconnectedCallback;
    void (*shadowDeltaCallback)(void *, AwsIotShadowCallbackParam_t *);
    void (*shadowUpdatedCallback)(void *, AwsIotShadowCallbackParam_t *);
} iot_connection_params_t;

esp_err_t lab_connection_init(iot_connection_params_t * params);
void lab_connection_ready_wait(void);
void lab_connection_cleanup(void);

esp_err_t lab_connection_update_shadow(AwsIotShadowDocumentInfo_t *updateDocument);
esp_err_t lab_connection_publish(IotMqttPublishInfo_t *publishInfo, IotMqttCallbackInfo_t *publishComplete);

#endif /* ifndef _LAB_CONNECTION_H_ */
