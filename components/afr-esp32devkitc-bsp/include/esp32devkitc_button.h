/**
 * esp32devkitc_button.h
 *
 * (C) 2020 - Timothee Cruse <timothee.cruse@gmail.com>
 * This code is licensed under the MIT License.
 */

#ifndef _ESP32DEVKITC_BUTTON_H_
#define _ESP32DEVKITC_BUTTON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_system.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp32devkitc_event.h"

#define ESP32DEVKITC_BUTTON_GPIO      GPIO_NUM_0

ESP_EVENT_DECLARE_BASE(ESP32DEVKITC_BUTTON_EVENT_BASE); /*!< BASE event of button */

#define ESP32DEVKITC_BUTTON_TASK_STACK_DEPTH   2048

/*!< event_group flag bits */
#define ESP32DEVKITC_BUTTON_PUSH_BIT      0b00000001
#define ESP32DEVKITC_BUTTON_POP_BIT       0b00000010

/*!< Time constants */
#define ESP32DEVKITC_BUTTON_DEBOUNCE_TIME   10
#define ESP32DEVKITC_BUTTON_HOLD_TIME       2000

/**
 * List of possible events this module can trigger
 */
typedef enum {
    ESP32DEVKITC_BUTTON_CLICK_EVENT = 0,        /*!< Normal button press */
    ESP32DEVKITC_BUTTON_HOLD_EVENT,             /*!< Button hold */
    ESP32DEVKITC_BUTTON_EVENT_MAX
} esp32devkitc_button_event_id_t;

typedef struct {
    gpio_num_t gpio;                                    /*!< Button GPIO number */
    uint32_t debounce_time;                             /*!< Button debounce time */
    uint32_t hold_time;                                 /*!< Button hold time */
    esp_event_base_t esp_event_base;                    /*!< Button event base */
    EventGroupHandle_t event_group;                     /*!< Event group handle */
    TaskHandle_t task;                                  /*!< Button task handle */
    #if defined(CONFIG_SUPPORT_STATIC_ALLOCATION)
    StaticEventGroup_t event_group_buffer;              /*!< Event group buffer for static allocation */
    StaticTask_t task_buffer;                           /*!< Task buffer for static allocation */
    StackType_t task_stack[ESP32DEVKITC_BUTTON_TASK_STACK_DEPTH];  /*!< Task stack for static allocation */
    #endif // STATIC_ALLOCATION
} esp32devkitc_button_t;

extern esp32devkitc_button_t esp32devkitc_button;       /*!< Button is the button on the front of M5StickC */

/**
 * @brief   Button interrupt service routine.
 */
void IRAM_ATTR esp32devkitc_button_isr_handler(void* arg);

/**
 * @brief   Initialize buttons
 *
 *          Initializes common resources shared by all buttons. It does not enable any buttons.
 *
 * @return  ESP_OK success
 *          ESP_FAIL failed
 */
esp_err_t eESP32DevkitcButtonInit();

/**
 * @brief   Enable button logic, needed to use the button.
 *
 * @param   button button to enable
 * @return  ESP_OK success
 *          ESP_FAIL failed
 */
esp_err_t eESP32DevkitcButtonEnable(esp32devkitc_button_t * button);

/**
 * @brief   Disable button logic.
 *
 * @param   button button to disable
 * @return  ESP_OK success
 *          ESP_FAIL failed
 *          ESP_ERR_INVALID_ARG button null
 */
esp_err_t eESP32DevkitcButtonDisable(esp32devkitc_button_t * button);

/**
 * @brief   Set button's GPIO as input.
 *
 * @param   button button to enable
 * @return  ESP_OK success
 *          ESP_FAIL failed
 *          ESP_ERR_INVALID_ARG button null
 */
esp_err_t eESP32DevkitcButtonSetAsInput(esp32devkitc_button_t * button);

/**
 * @brief   Enable interrupt for a button, needed to use events.
 *
 * @param   button button to enable interrupt of
 * @return  ESP_OK success
 *          ESP_FAIL failed
 *          ESP_ERR_INVALID_ARG button null
 */
esp_err_t eESP32DevkitcButtonEnableInterrupt(esp32devkitc_button_t * button);

/**
 * @brief   Disable interrupt for a button.
 *
 * @param   button button to disable interrupt of
 * @return  ESP_OK success
 *          ESP_FAIL failed
 *          ESP_ERR_INVALID_ARG button null
 */
esp_err_t eESP32DevkitcButtonDisableInterrupt(esp32devkitc_button_t * button);

/**
 * @brief   Check if button is pressed
 *
 * @param   button button to check
 * @return  false not pressed
 *          true otherwise pressed
 */
bool bIsESP32DevkitcButtonPressed(esp32devkitc_button_t * button);

/**
 * @brief   Generates button events
 *
 *          This task generates button events such as press and release. It includes a debouncing mechanism.
 *          First, the task will wait for a key push or pop. When detected, it will "propose" a candidate event to be sent
 *          and configure a timer of 100ms. If the timer expires the candidate event is posted to the default loop. If
 *          button state changes before the timeout, it will propose the new state of button as candidate for event and
 *          restart timer again.
 *
 * @param   pvParameter pointer to esp32devkitc_button
 */
void vESP32DevkitcButtonTask(void * pvParameter);

#ifdef __cplusplus
}
#endif

#endif // _ESP32DEVKITC_BUTTON_H_