/**
 * esp32devkitc_button.c
 *
 * (C) 2019 - Timothee Cruse <timothee.cruse@gmail.com>
 * This code is licensed under the MIT License.
 */

#include "esp32devkitc_button.h"

static const char * TAG = "esp32devkitc_button";

ESP_EVENT_DEFINE_BASE(ESP32DEVKITC_BUTTON_EVENT_BASE);

esp32devkitc_button_t esp32devkitc_button = {
    .gpio = ESP32DEVKITC_BUTTON_GPIO,
    .debounce_time = ESP32DEVKITC_BUTTON_DEBOUNCE_TIME,
    .hold_time = ESP32DEVKITC_BUTTON_HOLD_TIME
};

void IRAM_ATTR esp32devkitc_button_isr_handler(void* arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    esp32devkitc_button_t * button = (esp32devkitc_button_t *) arg;

    if(gpio_get_level(button->gpio) == 0) {
        xEventGroupSetBitsFromISR(button->event_group, ESP32DEVKITC_BUTTON_PUSH_BIT, &xHigherPriorityTaskWoken);
    } else {
        xEventGroupSetBitsFromISR(button->event_group, ESP32DEVKITC_BUTTON_POP_BIT, &xHigherPriorityTaskWoken);
    }
}

esp_err_t eESP32DevkitcButtonInit()
{
    esp_err_t e;

    esp32devkitc_button.esp_event_base = ESP32DEVKITC_BUTTON_EVENT_BASE;

    #define ESP_INTR_FLAG_DEFAULT 0
    e = gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    if(e == ESP_ERR_INVALID_STATE) {
        ESP_LOGD(TAG, "ISR service already installed");
    } else if(e == ESP_ERR_NO_MEM || e == ESP_ERR_NOT_FOUND) {
        ESP_LOGE(TAG, "Error installing ISR service");
        return ESP_FAIL;
    }

    e = eESP32DevkitcButtonEnable(&esp32devkitc_button);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Button enabled");
    } else {
        ESP_LOGE(TAG, "Error enabling button");
    }


    if(e != ESP_OK) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t eESP32DevkitcButtonEnable(esp32devkitc_button_t * button)
{
    esp_err_t e;

    if(button == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Set gpio as input
    e = eESP32DevkitcButtonSetAsInput(button);
    if(e != ESP_OK) {
        return ESP_FAIL;
    }

    // Init event_group
    #if defined(CONFIG_SUPPORT_STATIC_ALLOCATION)
    button->event_group = xEventGroupCreateStatic(&(button->event_group_buffer));
    #else
    button->event_group = xEventGroupCreate();
    #endif
    if(button->event_group == NULL) {
        ESP_LOGE(TAG, "Error creating button event group");
        return ESP_FAIL;
    }

    // Start task
    #if defined(CONFIG_SUPPORT_STATIC_ALLOCATION)
    button->task = xTaskCreateStatic(vESP32DevkitcButtonTask, "button_task", ESP32DEVKITC_BUTTON_TASK_STACK_DEPTH, (void *) button, 20, button->task_stack, &(button->task_buffer));
    if(button->task == NULL) {
        ESP_LOGE(TAG, "Error creating button_task");
        vEventGroupDelete(button->event_group);
        return ESP_FAIL;
    }
    #else
    BaseType_t r = xTaskCreate(vESP32DevkitcButtonTask, "button_task", ESP32DEVKITC_BUTTON_TASK_STACK_DEPTH, (void *) button, 20, &(button->task));
    if(r != pdPASS) {
        ESP_LOGE(TAG, "Error creating button_task");
        vEventGroupDelete(button->event_group);
        return ESP_FAIL;
    }
    #endif

    // Set interrupt type
    e = gpio_set_intr_type(button->gpio, GPIO_INTR_ANYEDGE);
    if(e != ESP_OK) {
        return e;
    }

    // Enable interrupt
    e = eESP32DevkitcButtonEnableInterrupt(button);
    if(e != ESP_OK) {
        vTaskDelete(button->task);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t eESP32DevkitcButtonDisable(esp32devkitc_button_t * button)
{
    if(button == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    vEventGroupDelete(button->event_group);
    vTaskDelete(button->task);

    return ESP_OK;
}

esp_err_t eESP32DevkitcButtonSetAsInput(esp32devkitc_button_t * button)
{
    esp_err_t e;

    gpio_pad_select_gpio(button->gpio);
    e = gpio_set_direction(button->gpio, GPIO_MODE_INPUT);
    if(e != ESP_OK) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t eESP32DevkitcButtonEnableInterrupt(esp32devkitc_button_t * button)
{
    esp_err_t e;

    e = gpio_isr_handler_add(button->gpio, esp32devkitc_button_isr_handler, button);
    if(e != ESP_OK) {
        return e;
    }

    return ESP_OK;
}

esp_err_t eESP32DevkitcButtonDisableInterrupt(esp32devkitc_button_t * button)
{
    return gpio_isr_handler_remove(button->gpio);
}

bool bIsESP32DevkitcButtonPressed(esp32devkitc_button_t * button)
{
    return (gpio_get_level(button->gpio) == 0) ? true : false;
}

void vESP32DevkitcButtonTask(void * pvParameter)
{
    EventBits_t event;
    esp32devkitc_button_t * button = (esp32devkitc_button_t *) pvParameter;

    ESP_LOGD(TAG, "Button task started");

    while(1) {
        event = xEventGroupWaitBits(button->event_group, ESP32DEVKITC_BUTTON_PUSH_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
        if((event & ESP32DEVKITC_BUTTON_PUSH_BIT) != 0) {
            vTaskDelay(button->debounce_time/portTICK_PERIOD_MS);
            xEventGroupClearBits(button->event_group, ESP32DEVKITC_BUTTON_POP_BIT);
            event = xEventGroupWaitBits(button->event_group, ESP32DEVKITC_BUTTON_POP_BIT, pdTRUE, pdFALSE, button->hold_time / portTICK_PERIOD_MS);
            if((event & ESP32DEVKITC_BUTTON_POP_BIT) != 0) {
                esp_event_post_to(esp32devkitc_event_loop, button->esp_event_base, ESP32DEVKITC_BUTTON_CLICK_EVENT, NULL, 0, portMAX_DELAY);
                ESP_LOGD(TAG, "BUTTON_CLICK event");
            } else {
                esp_event_post_to(esp32devkitc_event_loop, button->esp_event_base, ESP32DEVKITC_BUTTON_HOLD_EVENT, NULL, 0, portMAX_DELAY);
                ESP_LOGD(TAG, "BUTTON_HOLD event");
            }
        }
    }
}