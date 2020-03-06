/**
 * @file lab_connection.c
 * @brief Connection code for the library to be used commonly accross the different labs.
 *
 * (C) 2019 - Timothee Cruse <timothee.cruse@gmail.com>
 * This code is licensed under the MIT License.
 */

/* The config header is always included first. */
#include "iot_config.h"

/* Standard includes. */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Set up logging for this demo. */
#include "iot_demo_logging.h"

/* Platform layer includes. */
#include "platform/iot_clock.h"
#include "platform/iot_threads.h"

/* MQTT include. */
#include "iot_mqtt.h"

/* Shadow include. */
#include "aws_iot_shadow.h"

/* JSON utilities include. */
#include "iot_json_utils.h"

#include "aws_demo.h"
#include "types/iot_network_types.h"
#include "esp_log.h"

#include "iot_wifi.h"
#include "iot_ble_config.h"

#include "aws_iot_network_config.h"

#include "device.h"
#include "lab_config.h"
#include "lab_connection.h"

/*-----------------------------------------------------------*/

static const char *TAG = "lab_connection";

/*-----------------------------------------------------------*/

#define IOT_MQTT_TOPIC_PREFIX "mydevice"

/**
 * @brief The timeout for MQTT operations.
 */
#define MQTT_TIMEOUT_MS (5000)

/**
 * @brief The first characters in the client identifier. A timestamp is appended
 * to this prefix to create a unique client identifer.
 *
 * This prefix is also used to generate topic names and topic filters used.
 */
#define CLIENT_IDENTIFIER_PREFIX "mydevice"

/**
 * @brief The longest client identifier that an MQTT server must accept (as defined
 * by the MQTT 3.1.1 spec) is 23 characters. Add 1 to include the length of the NULL
 * terminator.
 */
#define CLIENT_IDENTIFIER_MAX_LENGTH (24)

/**
 * @brief The keep-alive interval.
 *
 * An MQTT ping request will be sent periodically at this interval.
 */
#define KEEP_ALIVE_SECONDS (60)

/**
 * @brief The Last Will and Testament topic name.
 *
 * The MQTT server will publish a message to this topic name if this client is
 * unexpectedly disconnected.
 */
#define LWT_TOPIC_NAME_FORMAT IOT_MQTT_TOPIC_PREFIX "/%s/lwt"

/**
 * @brief The length of #LWT_TOPIC_NAME.
 */
#define LWT_TOPIC_NAME_MAX_LENGTH ((uint16_t)(sizeof(LWT_TOPIC_NAME_FORMAT) - 1 + 12))

/**
 * @brief The message to publish to #LWT_TOPIC_NAME.
 */
#define LWT_MESSAGE "{\"message\": \"disconnected\"}"

/**
 * @brief The length of #LWT_MESSAGE.
 */
#define LWT_MESSAGE_LENGTH ((size_t)(sizeof(LWT_MESSAGE) - 1))

/*-----------------------------------------------------------*/

/* Semaphore for connection readiness */
static IotSemaphore_t connectionReadySem;

/* Semaphore for connection library / SDK clean up */
static IotSemaphore_t cleanUpReadySem;

/* Semaphore for shadow delta management */ 
// static IotSemaphore_t shadowDeltaSem;

/* Handle of the MQTT connection used in this demo. */
static IotMqttConnection_t _mqttConnection = IOT_MQTT_CONNECTION_INITIALIZER;

static iot_connection_params_t *_pConnectionParams = NULL;

static bool mqttConnectionEstablished = false;

ESP_EVENT_DEFINE_BASE(LAB_CONNECTION_EVENT_BASE);
esp_event_loop_handle_t lab_connection_event_loop;

char prvThingName[128] = { 0 }; 

/*-----------------------------------------------------------*/

void vNetworkConnectedCallback( bool awsIotMqttMode,
                                const char * pIdentifier,
                                void * pNetworkServerInfo,
                                void * pNetworkCredentialInfo,
                                const IotNetworkInterface_t * pNetworkInterface )
{
    esp_event_post_to(lab_connection_event_loop, 
                    LAB_CONNECTION_EVENT_BASE, 
                    LABCONNECTION_NETWORK_CONNECTED, 
                    (void *)pIdentifier, 
                    pIdentifier == NULL ? 0 : strlen(pIdentifier), 
                    portMAX_DELAY);    
}

void vNetworkDisconnectedCallback( const IotNetworkInterface_t * pNetworkInterface )
{
    esp_event_post_to(lab_connection_event_loop, 
                    LAB_CONNECTION_EVENT_BASE, 
                    LABCONNECTION_NETWORK_DISCONNECTED, 
                    NULL, 0, 
                    portMAX_DELAY);    
}

void vMQTTDisconnectedCallback( void * pCallbackContext, IotMqttCallbackParam_t * pIotMqttCallbackParam )
{
    esp_event_post_to(lab_connection_event_loop, 
                    LAB_CONNECTION_EVENT_BASE, 
                    LABCONNECTION_MQTT_DISCONNECTED, 
                    NULL, 0, 
                    portMAX_DELAY);    
}

void prvLabConnectionEventHandler(void * handler_arg, esp_event_base_t base, int32_t id, void * event_data)
{
    if (id == LABCONNECTION_NETWORK_CONNECTED)
    {
        ESP_LOGI(TAG, "LABCONNECTION_NETWORK_CONNECTED");
        STATUS_LED_ON();
    }
    else if (id == LABCONNECTION_NETWORK_DISCONNECTED)
    {
        ESP_LOGI(TAG, "LABCONNECTION_NETWORK_DISCONNECTED");
        STATUS_LED_OFF();
    }
    else if (id == LABCONNECTION_MQTT_CONNECTED)
    {
        char * thingName = ((connection_event_params_t *)event_data)->thingName;
        ESP_LOGI(TAG, "LABCONNECTION_MQTT_CONNECTED: %s (%i)", thingName, strlen(thingName));
    }
    else if (id == LABCONNECTION_MQTT_DISCONNECTED)
    {
        ESP_LOGI(TAG, "LABCONNECTION_MQTT_DISCONNECTED");
        vLabConnectionCleanup();
    }
}

/*-----------------------------------------------------------*/

/**
 * @brief Initialize the the MQTT library and the Shadow library.
 *
 * @return `EXIT_SUCCESS` if all libraries were successfully initialized;
 * `EXIT_FAILURE` otherwise.
 */
static int _initializeLibraries(void)
{
    int status = EXIT_SUCCESS;
    IotMqttError_t mqttInitStatus = IOT_MQTT_SUCCESS;
    AwsIotShadowError_t shadowInitStatus = AWS_IOT_SHADOW_SUCCESS;

    /* Flags to track cleanup on error. */
    bool mqttInitialized = false;

    /* Initialize the MQTT library. */
    mqttInitStatus = IotMqtt_Init();

    if (mqttInitStatus == IOT_MQTT_SUCCESS)
    {
        mqttInitialized = true;
    }
    else
    {
        ESP_LOGE(TAG, "_initialize: Failed to init MQTT Library");
        status = EXIT_FAILURE;
    }

    /* Initialize the Shadow library. */
    if (status == EXIT_SUCCESS)
    {
        /* Use the default MQTT timeout. */
        shadowInitStatus = AwsIotShadow_Init(0);

        if (shadowInitStatus != AWS_IOT_SHADOW_SUCCESS)
        {
            ESP_LOGE(TAG, "_initialize: Failed to init Shadow Library");
            status = EXIT_FAILURE;
        }
    }

    /* Clean up on error. */
    if (status == EXIT_FAILURE)
    {
        if (mqttInitialized == true)
        {
            IotMqtt_Cleanup();
        }
    }

    return status;
}

/*-----------------------------------------------------------*/

/**
 * @brief Clean up the the MQTT library and the Shadow library.
 */
static void _cleanup(void)
{
    AwsIotShadow_Cleanup();
    IotMqtt_Cleanup();
}

/*-----------------------------------------------------------*/

/**
 * @brief Set the Shadow callback functions used in this demo.
 *
 * @param[in] pThingName The Thing Name for Shadows in this demo.
 *
 * @return `EXIT_SUCCESS` if all Shadow callbacks were set; `EXIT_FAILURE`
 * otherwise.
 */
static int _setShadowCallbacks(const char *pThingName)
{
    int status = EXIT_SUCCESS;
    AwsIotShadowError_t callbackStatus = AWS_IOT_SHADOW_STATUS_PENDING;
    AwsIotShadowCallbackInfo_t deltaCallback = AWS_IOT_SHADOW_CALLBACK_INFO_INITIALIZER;
    AwsIotShadowCallbackInfo_t updatedCallback = AWS_IOT_SHADOW_CALLBACK_INFO_INITIALIZER;

    /* Set the functions for callbacks. */
    // deltaCallback.pCallbackContext = &shadowDeltaSem;
    deltaCallback.function = _pConnectionParams->shadowDeltaCallback;
    updatedCallback.function = _pConnectionParams->shadowUpdatedCallback;

    if (_pConnectionParams->shadowDeltaCallback != NULL)
    {
        /* Set the delta callback, which notifies of different desired and reported
         * Shadow states. */
        callbackStatus = AwsIotShadow_SetDeltaCallback(_mqttConnection,
                                                    pThingName,
                                                    strlen(pThingName),
                                                    0,
                                                    &deltaCallback);

        if (callbackStatus != AWS_IOT_SHADOW_SUCCESS)
        {
            IotLogError("Failed to set shadow callback, error %s.",
                        AwsIotShadow_strerror(callbackStatus));
            status = EXIT_FAILURE;
        }
        else
        {
            IotLogInfo("Successfully set delta callbacks");
        }
        
    }

    if (_pConnectionParams->shadowUpdatedCallback != NULL && callbackStatus == AWS_IOT_SHADOW_SUCCESS)
    {
        /* Set the updated callback, which notifies when a Shadow document is
         * changed. */
        callbackStatus = AwsIotShadow_SetUpdatedCallback(_mqttConnection,
                                                         pThingName,
                                                         strlen(pThingName),
                                                         0,
                                                         &updatedCallback);
        if (callbackStatus != AWS_IOT_SHADOW_SUCCESS)
        {
            IotLogError("Failed to set shadow callback, error %s.",
                        AwsIotShadow_strerror(callbackStatus));
            status = EXIT_FAILURE;
        }
        else
        {
            IotLogInfo("Successfully set updated callbacks");
        }
    }

    return status;
}

/*-----------------------------------------------------------*/

/**
 * @brief Establish a new connection to the MQTT server.
 *
 * @param[in] pIdentifier NULL-terminated MQTT client identifier. The Shadow
 * demo will use the Thing Name as the client identifier.
 * @param[in] pNetworkServerInfo Passed to the MQTT connect function when
 * establishing the MQTT connection.
 * @param[in] pNetworkCredentialInfo Passed to the MQTT connect function when
 * establishing the MQTT connection.
 * @param[in] pNetworkInterface The network interface to use for this demo.
 *
 * @return `EXIT_SUCCESS` if the connection is successfully established; `EXIT_FAILURE`
 * otherwise.
 */
static int _establishMqttConnection(const char *pIdentifier,
                                    void *pNetworkServerInfo,
                                    void *pNetworkCredentialInfo,
                                    const IotNetworkInterface_t *pNetworkInterface)
{
    int status = EXIT_SUCCESS;
    IotMqttError_t connectStatus = IOT_MQTT_STATUS_PENDING;
    IotMqttNetworkInfo_t networkInfo = IOT_MQTT_NETWORK_INFO_INITIALIZER;
    IotMqttConnectInfo_t connectInfo = IOT_MQTT_CONNECT_INFO_INITIALIZER;
    IotMqttPublishInfo_t lwtInfo = IOT_MQTT_PUBLISH_INFO_INITIALIZER;
    IotMqttCallbackInfo_t disconnectInfo = {
        .pCallbackContext = NULL,
        .function = vMQTTDisconnectedCallback
    };
    char pClientIdentifierBuffer[CLIENT_IDENTIFIER_MAX_LENGTH] = {0};
    char pLwtBuffer[LWT_TOPIC_NAME_MAX_LENGTH] = {0};

    /* Set the members of the network info not set by the initializer. This
     * struct provided information on the transport layer to the MQTT connection. */
    networkInfo.createNetworkConnection = true;
    networkInfo.u.setup.pNetworkServerInfo = pNetworkServerInfo;
    networkInfo.u.setup.pNetworkCredentialInfo = pNetworkCredentialInfo;
    networkInfo.pNetworkInterface = pNetworkInterface;
    networkInfo.disconnectCallback = disconnectInfo;

    #if (IOT_MQTT_ENABLE_SERIALIZER_OVERRIDES == 1) && defined(IOT_DEMO_MQTT_SERIALIZER)
        networkInfo.pMqttSerializer = IOT_DEMO_MQTT_SERIALIZER;
    #endif

    /* Set the members of the connection info not set by the initializer. */
    connectInfo.awsIotMqttMode = true;
    connectInfo.cleanSession = true;
    connectInfo.keepAliveSeconds = KEEP_ALIVE_SECONDS;
    connectInfo.pWillInfo = &lwtInfo;

    status = snprintf(pLwtBuffer,
                        LWT_TOPIC_NAME_MAX_LENGTH,
                        LWT_TOPIC_NAME_FORMAT,
                        _pConnectionParams->strID);

    /* Check for errors from snprintf. */
    if (status < 0)
    {
        ESP_LOGE(TAG, "Failed to generate the LWT topic name.");
        return EXIT_FAILURE;
    }
    else
    {
        status = EXIT_SUCCESS;

        /* Set the members of the Last Will and Testament (LWT) message info. The
         * MQTT server will publish the LWT message if this client disconnects
         * unexpectedly. */
        lwtInfo.pTopicName = pLwtBuffer;
        lwtInfo.topicNameLength = strlen(pLwtBuffer);
        lwtInfo.pPayload = LWT_MESSAGE;
        lwtInfo.payloadLength = LWT_MESSAGE_LENGTH;
    }

    if (_pConnectionParams->useShadow == true && pIdentifier == NULL)
    {
        ESP_LOGE(TAG, "Shadow Thing Name must be provided.");
        return EXIT_FAILURE;
    }

    /* Use the parameter client identifier if provided. Otherwise, generate a
     * unique client identifier. */
    if (pIdentifier != NULL)
    {
        connectInfo.pClientIdentifier = pIdentifier;
        connectInfo.clientIdentifierLength = (uint16_t)strlen(pIdentifier);
    }
    else
    {
        /* Every active MQTT connection must have a unique client identifier. The demos
         * generate this unique client identifier by appending a timestamp to a common
         * prefix. */
        status = snprintf(pClientIdentifierBuffer,
                            CLIENT_IDENTIFIER_MAX_LENGTH,
                            CLIENT_IDENTIFIER_PREFIX "%lu",
                            (long unsigned int)IotClock_GetTimeMs());

        /* Check for errors from snprintf. */
        if (status < 0)
        {
            IotLogError("Failed to generate unique client identifier for demo.");
            return EXIT_FAILURE;
        }
        else
        {
            /* Set the client identifier buffer and length. */
            connectInfo.pClientIdentifier = pClientIdentifierBuffer;
            connectInfo.clientIdentifierLength = (uint16_t)status;

            status = EXIT_SUCCESS;
        }
    }

    /* Establish the MQTT connection. */
    if ( status == EXIT_SUCCESS )
    {
        IotLogInfo("MQTT client identifier is %.*s (length %hu).",
                   connectInfo.clientIdentifierLength,
                   connectInfo.pClientIdentifier,
                   connectInfo.clientIdentifierLength);

        connectStatus = IotMqtt_Connect(&networkInfo,
                                        &connectInfo,
                                        MQTT_TIMEOUT_MS,
                                        &_mqttConnection);

        if (connectStatus != IOT_MQTT_SUCCESS)
        {
            IotLogError("MQTT CONNECT returned error %s.", IotMqtt_strerror(connectStatus));
            status = EXIT_FAILURE;
        }
        else
        {

            uint8_t thingNameLength = ((connectInfo.clientIdentifierLength) < (128) ? (connectInfo.clientIdentifierLength) : (128));
            strncpy(prvThingName, connectInfo.pClientIdentifier, thingNameLength);

            IotLogInfo("MQTT CONNECT Success for %s (%i)", prvThingName, strlen(prvThingName));

            status = IOT_MQTT_SUCCESS;
        }
    }

    return status;
}

/*-----------------------------------------------------------*/

uint32_t _getSavedWifiNetworks( void )
{
    uint32_t idx;
    WIFIReturnCode_t WifiRet;
    WIFINetworkProfile_t profile;

    ESP_LOGI(TAG, "Reading stored WIFI creds:");

    for( idx = 0; idx < IOT_BLE_WIFI_PROVISIONING_MAX_SAVED_NETWORKS; idx++ )
    {
        WifiRet = WIFI_NetworkGet( &profile, idx );

        if( WifiRet != eWiFiSuccess )
        {
            break;
        }

        ESP_LOGI(TAG, "  - %d: %s", idx, profile.cSSID);
    }

    return idx;
}

/*-----------------------------------------------------------*/

void vLabConnectionResetWifiNetworks( void )
{
    uint32_t idx;
    WIFIReturnCode_t WifiRet;
    WIFINetworkProfile_t profile;

    ESP_LOGI(TAG, "Deleting stored WIFI creds:");

    for( idx = 0; idx < IOT_BLE_WIFI_PROVISIONING_MAX_SAVED_NETWORKS; idx++ )
    {
        WifiRet = WIFI_NetworkGet( &profile, idx );

        if( WifiRet != eWiFiSuccess )
        {
            break;
        }

        ESP_LOGI(TAG, "Deleting WIFI - %d: %s", idx, profile.cSSID);

        WifiRet = WIFI_NetworkDelete( idx );

        if( WifiRet != eWiFiSuccess )
        {
            break;
        }
    }
}

/*-----------------------------------------------------------*/

/**
 * @brief The function that runs the MQTT demo, called by the demo runner.
 *
 * @param[in] awsIotMqttMode Specify if this demo is running with the AWS IoT
 * MQTT server. Set this to `false` if using another MQTT server.
 * @param[in] pIdentifier NULL-terminated MQTT client identifier.
 * @param[in] pNetworkServerInfo Passed to the MQTT connect function when
 * establishing the MQTT connection.
 * @param[in] pNetworkCredentialInfo Passed to the MQTT connect function when
 * establishing the MQTT connection.
 * @param[in] pNetworkInterface The network interface to use for this demo.
 *
 * @return `EXIT_SUCCESS` if the demo completes successfully; `EXIT_FAILURE` otherwise.
 */

int lab_run(bool awsIotMqttMode,
            const char *pIdentifier,
            void *pNetworkServerInfo,
            void *pNetworkCredentialInfo,
            const IotNetworkInterface_t *pNetworkInterface)
{
    static connection_event_params_t connectionEventParams;

    for(;;)
    {        
        /* Return value of this function and the exit status of this program. */
        int status = EXIT_SUCCESS;

        /* Flags for tracking which cleanup functions must be called. */
        bool librariesInitialized = false;
        
        mqttConnectionEstablished = false;

        /* The first parameter of this demo function is not used. Shadows are specific
        * to AWS IoT, so this value is hardcoded to true whenever needed. */
        (void)awsIotMqttMode;

        _getSavedWifiNetworks();

        ESP_LOGI(TAG, "lab_run: Start");

        /* Initialize the libraries required for this demo. */
        status = _initializeLibraries();

        if (status == EXIT_SUCCESS)
        {
            /* Mark the libraries as initialized. */
            librariesInitialized = true;

            ESP_LOGI(TAG, "lab_run: Libraries initialized");

            /* Establish a new MQTT connection. */
            status = _establishMqttConnection(pIdentifier,
                                            pNetworkServerInfo,
                                            pNetworkCredentialInfo,
                                            pNetworkInterface);
        }
        else
        {
            ESP_LOGE(TAG, "lab_run: Failed to initialize the libraries: %i", status);
            return status;
        }

        if (status == EXIT_SUCCESS)
        {
            /* Mark the MQTT connection as established. */
            mqttConnectionEstablished = true;

            ESP_LOGI(TAG, "lab_run: MQTT Connection established");

            /* Set the Shadow callbacks for this demo. */
            status = _setShadowCallbacks(prvThingName);
        }
        else
        {
            ESP_LOGE(TAG, "lab_run: Failed to initialize the MQTT Connection: %i", status);
        }

        if (status == EXIT_SUCCESS)
        {

            connectionEventParams.thingName = prvThingName;

            if (ESP_OK != esp_event_post_to(lab_connection_event_loop, 
                                        LAB_CONNECTION_EVENT_BASE, 
                                        LABCONNECTION_MQTT_CONNECTED, 
                                        &connectionEventParams,
                                        sizeof(connection_event_params_t),
                                        portMAX_DELAY))
            {
                ESP_LOGE(TAG, "lab_run: failed to post to event");
            }

            // Connection is ready
            IotSemaphore_Post(&connectionReadySem);
            // Shadow can be used
            // IotSemaphore_Post(&shadowDeltaSem);

            ESP_LOGI(TAG, "lab_run: Waiting for connection clean up signal...");
            // Wait for clean up semaphore
            IotSemaphore_Wait(&cleanUpReadySem);

            ESP_LOGI(TAG, "lab_run: Received connection clean up signal.");
            
            // Cleanup connection cleanup semaphore
            IotSemaphore_Destroy(&cleanUpReadySem);
            // Cleanup shadow delta semaphore
            // IotSemaphore_Destroy(&shadowDeltaSem);        
        }

        /* Disconnect the MQTT connection if it was established. */
        if (mqttConnectionEstablished == true)
        {
            IotMqtt_Disconnect(_mqttConnection, 0);
            mqttConnectionEstablished = false;
        }

        /* Clean up libraries if they were initialized. */
        if (librariesInitialized == true)
        {
            ESP_LOGI(TAG, "lab_run: Cleaning up");
            _cleanup();
        }

        ESP_LOGI(TAG, "lab_run: End, looping in 10secs");
        vTaskDelay( pdMS_TO_TICKS( 10000 ) );

        return status;
    }
}

/*-----------------------------------------------------------*/

void vLabConnectionTask( void * pArgument )
{
    for(;;)
    {        
        ESP_LOGI(TAG, "Connection - Start");
        runDemoTask( pArgument );
        ESP_LOGI(TAG, "Connection - End (Looping in 5secs)");
        vTaskDelay( pdMS_TO_TICKS( 5000 ) );
    }
}

/*-----------------------------------------------------------*/

esp_err_t eLabConnectionInit(iot_connection_params_t * pConnectionParams)
{
    esp_err_t res = ESP_OK;

    _pConnectionParams = pConnectionParams;

    static demoContext_t mqttDemoContext =
    {
        .networkTypes = AWSIOT_NETWORK_TYPE_WIFI,
        .demoFunction = lab_run,
        .networkConnectedCallback = vNetworkConnectedCallback,
        .networkDisconnectedCallback = vNetworkDisconnectedCallback
    };
    
    if ( _pConnectionParams->networkConnectedCallback != NULL )
    {
        mqttDemoContext.networkConnectedCallback = _pConnectionParams->networkConnectedCallback;
    }
    
    if ( _pConnectionParams->networkDisconnectedCallback != NULL )
    {
        mqttDemoContext.networkDisconnectedCallback = _pConnectionParams->networkDisconnectedCallback;
    }

    esp_event_loop_args_t loop_args = {
        .queue_size = 5,
        .task_name = "lab_connection_event_loop",
        .task_priority = 10,
        .task_stack_size = 2048,
        .task_core_id = 0
    };

    res = esp_event_loop_create(&loop_args, &lab_connection_event_loop);
    if(res == ESP_OK) {
        ESP_LOGD(TAG, "Event loop created");
        if( ESP_OK == eLabConnectionRegisterCallback(prvLabConnectionEventHandler) ) {
            ESP_LOGD(TAG, "Network event handler registered");
        } else {
            ESP_LOGE(TAG, "Error registring the event handler: %s", esp_err_to_name(res));
            res = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "Error creating event loop: %s", esp_err_to_name(res));
    }

    // Create semaphore for connection readiness
    if (res == ESP_OK && !IotSemaphore_Create(&connectionReadySem, 0, 1))
    {
        ESP_LOGE(TAG, "Failed to create connection semaphore!");
        res = ESP_FAIL;
    }
    
    // Create semaphore for connection readiness
    if ( res == ESP_OK && !IotSemaphore_Create(&cleanUpReadySem, 0, 1))
    {
        ESP_LOGE(TAG, "Failed to create clean up semaphore!");
        res = ESP_FAIL;
    }

    // Create semaphore for shadow delta
    // if ( res == ESP_OK && !IotSemaphore_Create(&shadowDeltaSem, 0, 1))
    // {
    //     ESP_LOGE(TAG, "Failed to create shadow delta semaphore!");
    //     res = ESP_FAIL;
    // }

    if ( res == ESP_OK )
    {
        ESP_LOGI(TAG, "Creating IoT Thread");
        if ( Iot_CreateDetachedThread(vLabConnectionTask, &mqttDemoContext, tskIDLE_PRIORITY + 5, configMINIMAL_STACK_SIZE * 8) )
        {
            res = ESP_OK;
        }
        else 
        {
            ESP_LOGE(TAG, "Failed to create IoT thread!");
            res = ESP_FAIL;
        }
    }

    return res;
}

/*-----------------------------------------------------------*/

// Shadow update completion callback.
void _updateComplete( void * pCallbackContext,
                      AwsIotShadowCallbackParam_t * pCallbackParam )
{
    ESP_LOGI(TAG, "_updateComplete");
}

esp_err_t eLabConnectionUpdateShadow(AwsIotShadowDocumentInfo_t *updateDocument)
{
    int status = EXIT_SUCCESS;
    AwsIotShadowError_t updateStatus = AWS_IOT_SHADOW_STATUS_PENDING;    
    AwsIotShadowCallbackInfo_t updateCallback = AWS_IOT_SHADOW_CALLBACK_INFO_INITIALIZER;
    updateCallback.function = _updateComplete;

    updateStatus = AwsIotShadow_Update(_mqttConnection, updateDocument, AWS_IOT_SHADOW_FLAG_KEEP_SUBSCRIPTIONS,
                                        &updateCallback, NULL);

    /* Check the status of the Shadow update. */
    if (updateStatus != AWS_IOT_SHADOW_STATUS_PENDING)
    {
        ESP_LOGE(TAG, "Failed to send Shadow update, error %s.", AwsIotShadow_strerror(updateStatus));
        status = EXIT_FAILURE;
    }
    else
    {
        ESP_LOGI(TAG, "Successfully queued Shadow update.");
    }

    return status;
}

/*-----------------------------------------------------------*/

esp_err_t eLabConnectionPublish(IotMqttPublishInfo_t * publishInfo, IotMqttCallbackInfo_t * publishComplete)
{
    int status = EXIT_SUCCESS;
    IotMqttError_t publishStatus = IOT_MQTT_STATUS_PENDING;

    if (NULL != _mqttConnection)
    {
        /* PUBLISH a message. This is an asynchronous function that notifies of
         * completion through a callback. */
        ESP_LOGI(TAG, "MQTT Publish: %s: %s", (char *)publishInfo->pTopicName, (char *)publishInfo->pPayload);

        publishStatus = IotMqtt_Publish(_mqttConnection, publishInfo, 0, publishComplete, NULL);

        if (publishStatus != IOT_MQTT_STATUS_PENDING)
        {
            ESP_LOGE(TAG, "MQTT Publish returned error %s.", IotMqtt_strerror(publishStatus));
            status = EXIT_FAILURE;
        }
    }
    else
    {
        ESP_LOGE(TAG, "MQTT Publish: MQTT Connection is (NULL) not available.");
        status = EXIT_FAILURE;
    }
        
    return status;
}
/*-----------------------------------------------------------*/

void vLabConnectionCleanup(void)
{
    IotSemaphore_Post(&cleanUpReadySem);
}


bool bIsLabConnectionMqttConnected(void)
{
    return mqttConnectionEstablished;
}

/*-----------------------------------------------------------*/

esp_err_t eLabConnectionRegisterCallback(void (*callback)(void * handler_arg, esp_event_base_t base, int32_t id, void * event_data) )
{
    esp_err_t res = ESP_FAIL;
    if (lab_connection_event_loop)
    {
        res = esp_event_handler_register_with(lab_connection_event_loop, LAB_CONNECTION_EVENT_BASE, ESP_EVENT_ANY_ID, callback, NULL);
        ESP_LOGI(TAG, "eLabConnectionRegisterCallback: Callback registered... %s", res == ESP_OK ? "OK" : "NOK");    
    }
    else
    {
        ESP_LOGE(TAG, "eLabConnectionRegisterCallback: lab_conenction_event_loop is NULL");
    }
    
    return res;
}

/*-----------------------------------------------------------*/
