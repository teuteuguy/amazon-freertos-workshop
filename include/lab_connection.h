/**
 * @file lab_connection.h
 *
 * (C) 2019 - Timothee Cruse <timothee.cruse@gmail.com>
 * This code is licensed under the MIT License.
 */

#ifndef _LAB_CONNECTION_H_
#define _LAB_CONNECTION_H_

#include "esp_err.h"
#include "esp_event.h"
#include "iot_mqtt.h"
#include "aws_iot_shadow.h"

/**
 * List of possible events this module can trigger
 */
typedef enum {
    LABCONNECTION_NETWORK_CONNECTED = 0,        /*!< Network connected */
    LABCONNECTION_NETWORK_DISCONNECTED,         /*!< Network disconnected */
    LABCONNECTION_MQTT_CONNECTED,               /*!< MQTT connected */
    LABCONNECTION_MQTT_DISCONNECTED,            /*!< MQTT disconnected */
    LABCONNECTION_EVENT_MAX
} lab_connection_event_id_t;

typedef struct {
    char * strID;
    bool useShadow;
    networkConnectedCallback_t networkConnectedCallback;
    networkDisconnectedCallback_t networkDisconnectedCallback;
    void (*shadowDeltaCallback)(void *, AwsIotShadowCallbackParam_t *);
    void (*shadowUpdatedCallback)(void *, AwsIotShadowCallbackParam_t *);
} iot_connection_params_t;

typedef struct {
    char * thingName;
} connection_event_params_t;

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

esp_err_t eLabConnectionRegisterCallback(void (*callback)(void * handler_arg, esp_event_base_t base, int32_t id, void * event_data) );

#endif /* ifndef _LAB_CONNECTION_H_ */
