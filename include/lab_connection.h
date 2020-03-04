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

typedef int (* labRunFunction_t)( bool awsIotMqttMode,
                                const char * pIdentifier,
                                void * pNetworkServerInfo,
                                void * pNetworkCredentialInfo,
                                const IotNetworkInterface_t * pNetworkInterface );

esp_err_t eLabConnectionInit(iot_connection_params_t * params);
void vLabConnectionCleanup(void);

esp_err_t eLabConnectionUpdateShadow(AwsIotShadowDocumentInfo_t *updateDocument);
esp_err_t eLabConnectionPublish(IotMqttPublishInfo_t *publishInfo, IotMqttCallbackInfo_t *publishComplete);

void vLabConnectionResetWifiNetworks( void );

bool bIsLabConnectionMqttConnected(void);

#endif /* ifndef _LAB_CONNECTION_H_ */
