/**
 * @file device.h
 *
 * (C) 2019 - Timothee Cruse <timothee.cruse@gmail.com>
 * This code is licensed under the MIT License.
 */

#ifndef _DEVICE_H_
#define _DEVICE_H_

#include "esp_event.h"
#include "esp_err.h"
#include "freertos/event_groups.h"
#include "driver/i2c.h"
#include "iot_i2c.h"

#include "lab_config.h"

#define DEVICE_STATUS_LED_ON()
#define DEVICE_STATUS_LED_OFF()

#define DISPLAY_PRINT( str, x, y )
#define DISPLAY_WIDTH
#define DISPLAY_HEIGHT

#define BUTTON_CLICK    0
#define BUTTON_HOLD     1

/**
 * @brief   DEVICE FAILURE CODES
 */
#define DEVICE_SUCCESS              ( 0 )           /** Operation completed successfully. */
#define DEVICE_FAIL                 ( 1 )           /** Operation failed. */
#define DEVICE_OTHER_ERROR          ( 2 )           /** Other error. */

/*!< event_group flag bits */
#define DEVICE_BUTTON_PUSH_BIT          0b00000001
#define DEVICE_BUTTON_POP_BIT           0b00000010

/*!< Time constants */
#define DEVICE_BUTTON_DEBOUNCE_TIME     10
#define DEVICE_BUTTON_HOLD_TIME         2000

/**
 * List of possible events this module can trigger
 */
typedef enum {
    DEVICE_BUTTON_CLICK_EVENT = 0,        /*!< Button click */
    DEVICE_BUTTON_HOLD_EVENT,             /*!< Button hold */
    DEVICE_EVENT_MAX
} device_events_t;

typedef int32_t device_err_t;

typedef struct 
{
    IotI2CHandle_t i2c_handler[ I2C_NUM_MAX ];
} device_t;

typedef struct
{
    gpio_num_t gpio;                                    /*!< Button GPIO number */
    uint32_t debounce_time;                             /*!< Button debounce time */
    uint32_t hold_time;                                 /*!< Button hold time */
    esp_event_base_t esp_event_base;                    /*!< Button event base */
    EventGroupHandle_t event_group;                     /*!< Event group handle */
    TaskHandle_t task;                                  /*!< Button task handle */
    #if defined(CONFIG_SUPPORT_STATIC_ALLOCATION)
        StaticEventGroup_t event_group_buffer;          /*!< Event group buffer for static allocation */
        StaticTask_t task_buffer;                       /*!< Task buffer for static allocation */
        StackType_t task_stack[ 2048 ];                 /*!< Task stack for static allocation */
    #endif // STATIC_ALLOCATION
} device_button_t;

extern esp_event_loop_handle_t device_event_loop;       /*!< Event loop for device-specific events */

#if defined(DEVICE_ESP32_DEVKITC)

    #include "esp32devkitc.h"

    #define DEVICE_HAS_MAIN_BUTTON      GPIO_NUM_0
    ESP_EVENT_DECLARE_BASE( BUTTON_MAIN_EVENT_BASE )    /*!< BASE event of button A */
    extern device_button_t device_button_a;             /*!< Button A */

#elif defined(DEVICE_M5STICKC)
    
    #define DEVICE_HAS_ACCELEROMETER
    #undef ADDON_MPU6886
    #define ADDON_MPU6886               I2C_NUM_0

    #define DEVICE_HAS_BATTERY

    #define DEVICE_HAS_STATUS_LED       GPIO_NUM_10
    #undef DEVICE_STATUS_LED_ON
    #undef DEVICE_STATUS_LED_OFF
    #define DEVICE_STATUS_LED_ON()      eDeviceSetLed( DEVICE_HAS_STATUS_LED, 0 )
    #define DEVICE_STATUS_LED_OFF()     eDeviceSetLed( DEVICE_HAS_STATUS_LED, 1 )

    #define DEVICE_HAS_MAIN_BUTTON      GPIO_NUM_37
    #define DEVICE_HAS_RESET_BUTTON     GPIO_NUM_39
    ESP_EVENT_DECLARE_BASE( BUTTON_MAIN_EVENT_BASE )    /*!< BASE event of button A */
    ESP_EVENT_DECLARE_BASE( BUTTON_RESET_EVENT_BASE )   /*!< BASE event of button B */
    extern device_button_t device_button_a;             /*!< Button A is the button on the front of M5StickC */
    extern device_button_t device_button_b;             /*!< Button B is the button on the side of M5StickC, far from the USB connector */

    // #undef DISPLAY_PRINT
    // #define DISPLAY_PRINT(str, x, y) TFT_print(str, x, y)
    // #undef DISPLAY_WIDTH
    // #define DISPLAY_WIDTH M5STICKC_DISPLAY_WIDTH
    // #undef DISPLAY_HEIGHT
    // #define DISPLAY_HEIGHT M5STICKC_DISPLAY_HEIGHT



    #include "axp192.h"
    #define AXP192_IRC_NUM              I2C_NUM_0

    // AXP192_REG_EXTEN_DCDC2_SWITCH_CONTROL
    #define CONFIG_AXP192_10H_BIT2 0x1
    #define CONFIG_AXP192_10H_BIT0 0x1

    // AXP192_REG_DCDC1_DCDC3_LDO2_LDO3_SWITCH_CONTROL
    #define CONFIG_AXP192_12H_BIT6      0x1
    #define CONFIG_AXP192_12H_BIT4      0x0 // 0x1
    #define CONFIG_AXP192_12H_BIT3      0x1
    #define CONFIG_AXP192_12H_BIT2      0x1 // 0x0
    #define CONFIG_AXP192_12H_BIT1      0x0 // 0x1
    #define CONFIG_AXP192_12H_BIT0      0x1

    // AXP192_REG_LDO2_LDO3_VOLTAGE_SETTING
    #define CONFIG_AXP192_28H_BIT7_4 0xC
    #define CONFIG_AXP192_28H_BIT3_0 0xC

    // AXP192_REG_VBUS_IPSOUT_PATH_SETTING
    #define CONFIG_AXP192_30H_BIT7      0x1
    #define CONFIG_AXP192_30H_BIT6      0x0 // 0x1
    #define CONFIG_AXP192_30H_BIT5_3    0x0 // 0x5
    #define CONFIG_AXP192_30H_BIT1      0x0
    #define CONFIG_AXP192_30H_BIT0      0x0

    // AXP192_REG_VOFF_SHUTDOWN_VOLTAGE_SETTING
    #define CONFIG_AXP192_31H_BIT3      0x0
    #define CONFIG_AXP192_31H_BIT2_0    0x4 // 0x0

    // AXP192_REG_OFF_BATTERY_DETECTION_CHGLED_CONTROL
    #define CONFIG_AXP192_32H_BIT7      0x0
    #define CONFIG_AXP192_32H_BIT6      0x1
    #define CONFIG_AXP192_32H_BIT5_4    0x0
    #define CONFIG_AXP192_32H_BIT3      0x0
    #define CONFIG_AXP192_32H_BIT2      0x1
    #define CONFIG_AXP192_32H_BIT1_0    0x2

    // AXP192_REG_CHARGE_CONTROL_1
    #define CONFIG_AXP192_33H_BIT7      0x1
    #define CONFIG_AXP192_33H_BIT6_5    0x2
    #define CONFIG_AXP192_33H_BIT4      0x0
    #define CONFIG_AXP192_33H_BIT3_0    0x0 // 0x1

    // AXP192_REG_BACKUP_BATTERY_CHARGING_CONTROL
    #define CONFIG_AXP192_35H_BIT7      0x1
    #define CONFIG_AXP192_35H_BIT6_5    0x1
    #define CONFIG_AXP192_35H_BIT1_0    0x2

    // AXP192_REG_PEK_SETTING
    #define CONFIG_AXP192_36H_BIT7_6    0x0
    #define CONFIG_AXP192_36H_BIT5_4    0x0
    #define CONFIG_AXP192_36H_BIT3      0x1
    #define CONFIG_AXP192_36H_BIT2      0x1
    #define CONFIG_AXP192_36H_BIT1_0    0x0

    // AXP192_REG_BATTERY_CHARGING_HIGH_TEMPERATURE_ALARM
    #define CONFIG_AXP192_39H_BIT7_0    0xFC

    // AXP192_REG_ADC_ENABLE_1
    #define CONFIG_AXP192_82H_BIT7      0x1
    #define CONFIG_AXP192_82H_BIT6      0x1
    #define CONFIG_AXP192_82H_BIT5      0x1
    #define CONFIG_AXP192_82H_BIT4      0x1
    #define CONFIG_AXP192_82H_BIT3      0x1
    #define CONFIG_AXP192_82H_BIT2      0x1
    #define CONFIG_AXP192_82H_BIT1      0x1
    #define CONFIG_AXP192_82H_BIT0      0x1

    // AXP192_REG_ADC_SAMPLE_RATE_TS_PIN_CONTROL
    #define CONFIG_AXP192_84H_BIT7      0x1
    #define CONFIG_AXP192_84H_BIT6      0x1
    #define CONFIG_AXP192_84H_BIT5_4    0x3
    #define CONFIG_AXP192_84H_BIT3      0x0
    #define CONFIG_AXP192_84H_BIT2      0x0
    #define CONFIG_AXP192_84H_BIT1_0    0x2

    // AXP192_REG_GPIO0_CONTROL
    #define CONFIG_AXP192_90H_BIT2_0    0x2

    // AXP192_REG_GPIO0_LDO_MODE_OUTPUT_VOLTAGE
    #define CONFIG_AXP192_91H_BIT7_4    0xF

    // AXP192_REG_COULOMB_COUNTER_CONTROL
    #define CONFIG_AXP192_B8H_BIT7      0x1
    #define CONFIG_AXP192_B8H_BIT6      0x0
    #define CONFIG_AXP192_B8H_BIT5      0x1

#endif

esp_err_t eDeviceInit( device_t * config );
esp_err_t eDeviceRegisterButtonCallback(esp_event_base_t base, void (*callback)(void * handler_arg, esp_event_base_t base, int32_t id, void * event_data) );
device_err_t eDeviceSetLed( uint8_t port, uint32_t value );

#endif /* ifndef _DEVICE_H_ */