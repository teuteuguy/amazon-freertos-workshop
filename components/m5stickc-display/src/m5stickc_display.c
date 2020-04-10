/**
 * m5stickc_display.c
 *
 * (C) 2020 - Timothee Cruse <timothee.cruse@gmail.com>
 * This code is licensed under the MIT License.
 */
#include "m5stickc_display.h"
#include "timers.h"

static const char * TAG = "m5stickc_display";

spi_lobo_device_handle_t m5stickc_display_spi;
TimerHandle_t m5stickc_display_timer;

esp_err_t M5StickCDisplayInit( void )
{
    esp_err_t e;

    tft_disp_type = M5STICKC_DISPLAY_TYPE;
    _width = 80;                                        // smaller dimension
    _height = 160;                                      // larger dimension    
    // max_rdclock = 8000000;
    max_rdclock = 25000000;

    TFT_PinsInit();

    spi_lobo_bus_config_t buscfg = {};
    buscfg.miso_io_num = PIN_NUM_MISO;				    // set SPI MISO pin
    buscfg.mosi_io_num = PIN_NUM_MOSI;				    // set SPI MOSI pin
    buscfg.sclk_io_num = PIN_NUM_CLK;				    // set SPI CLK pin
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = 6 * 1024;

    spi_lobo_device_interface_config_t devcfg = {};
    // devcfg.clock_speed_hz=8000000;                   // Initial clock out at 8 MHz
    devcfg.clock_speed_hz = 25000000;
    devcfg.mode=0;                                      // SPI mode 0
    devcfg.spics_io_num=-1;                             // we will use external CS pin
    devcfg.spics_ext_io_num=PIN_NUM_CS;                 // external CS pin
    devcfg.flags=LB_SPI_DEVICE_HALFDUPLEX;              // ALWAYS SET  to HALF DUPLEX MODE!! for display spi

    e = spi_lobo_bus_add_device(TFT_HSPI_HOST, &buscfg, &devcfg, &m5stickc_display_spi);
    if(e != ESP_OK) {
        ESP_LOGE(TAG, "Error adding display to SPI bus: %s", esp_err_to_name(e));
        return ESP_FAIL;
    }

    disp_spi = m5stickc_display_spi;

    // ==== Test select/deselect ====
    spi_lobo_device_select(m5stickc_display_spi, 1);
    spi_lobo_device_deselect(m5stickc_display_spi);

    // Pin reset
    gpio_set_level((gpio_num_t) PIN_NUM_RST, 1);
    vTaskDelay(5 / portTICK_PERIOD_MS);
    gpio_set_level((gpio_num_t) PIN_NUM_RST, 0);
    vTaskDelay(20 / portTICK_PERIOD_MS);
    gpio_set_level((gpio_num_t) PIN_NUM_RST, 1);

    TFT_display_init();
    TFT_invertDisplay(INVERT_ON);

    // ==== Set SPI clock used for display operations ====
    spi_lobo_set_speed(m5stickc_display_spi, DEFAULT_SPI_CLOCK);

    font_rotate = 0;
    text_wrap = 0;
    font_transparent = 0;
    font_forceFixed = 0;
    gray_scale = 0;
    TFT_setGammaCurve(DEFAULT_GAMMA_CURVE);
    TFT_setRotation(LANDSCAPE);
    TFT_setFont(DEFAULT_FONT, NULL);
    TFT_resetclipwin();

    return ESP_OK;
}

// esp_err_t M5StickCDisplaySetBacklightLevel(uint8_t backlight_level) {
//     esp_err_t e;
//     uint8_t register_content;

//     e = M5StickCPowerRegisterRead(LDO2_LDO3_VOLTAGE_SETTING_REG, &register_content);
//     if(e != ESP_OK) {
//         ESP_LOGE(TAG, "Error setting reading register to set backlight level: %s", esp_err_to_name(e));
//         return ESP_FAIL;
//     }

//     register_content &= 0x0F; // Clear 4 msb
//     register_content |= (0x80 | (backlight_level << 4)); // Set value. Values less than 0x80 are completely off.

//     e = M5StickCPowerRegisterWrite(LDO2_LDO3_VOLTAGE_SETTING_REG, register_content);
//     if(e != ESP_OK) {
//         ESP_LOGE(TAG, "Error writing register to set backlight level: %s", esp_err_to_name(e));
//         return ESP_FAIL;
//     }

//     return ESP_OK;
// }

// esp_err_t M5StickCDisplayTimeout(uint32_t timeout)
// {
//     esp_err_t e;

//     e = esp_event_handler_register_with(m5stickc_event_loop, M5STICKC_BUTTON_A_EVENT_BASE, ESP_EVENT_ANY_ID, M5StickCDisplayEventHandler, NULL);
//     if(e == ESP_OK) {
//         ESP_LOGD(TAG, "[ OK ] Registered for event BUTTON_A_CLICK");
//     } else {
//         ESP_LOGE(TAG, "[FAIL] Error registering for event BUTTON_A_CLICK");
//         return ESP_FAIL;
//     }

//     e = esp_event_handler_register_with(m5stickc_event_loop, M5STICKC_BUTTON_B_EVENT_BASE, ESP_EVENT_ANY_ID, M5StickCDisplayEventHandler, NULL);
//     if(e == ESP_OK) {
//         ESP_LOGD(TAG, "[ OK ] Registered for event BUTTON_B_CLICK");
//     } else {
//         ESP_LOGE(TAG, "[FAIL] Error registering for event BUTTON_B_CLICK");
//         return ESP_FAIL;
//     }

//     // Activate timer
//     m5stickc_display_timer = xTimerCreate("m5stickc_display_timer", timeout * 1000 / portTICK_PERIOD_MS, pdFALSE, (void *) 0, M5StickCDisplaySleep);
//     if(m5stickc_display_timer != NULL) {
//         ESP_LOGD(TAG, "[ OK ] Display timeout timer created");
//     } else {
//         ESP_LOGE(TAG, "[FAIL] Error creating display timeout timer");
//         return ESP_FAIL;
//     }

//     if(xTimerStart(m5stickc_display_timer, 0) == pdTRUE) {
//         ESP_LOGD(TAG, "[ OK ] Display timeout timer started");
//         return ESP_OK;
//     } else {
//         ESP_LOGE(TAG, "[FAIL] Error starting display timeout timer");
//         return ESP_FAIL;
//     }
// }

// void M5StickCDisplayWakeup()
// {
//     M5StickCDisplayOn();
//     if(xTimerReset(m5stickc_display_timer, 0) == pdTRUE) {
//         ESP_LOGD(TAG, "[ OK ] Display timeout timer reset");
//     } else {
//         ESP_LOGE(TAG, "[FAIL] Error resetting display timeout timer");
//     }
// }

// void M5StickCDisplaySleep()
// {
//     M5StickCDisplayOff();
// }

// void M5StickCDisplayEventHandler(void * handler_arg, esp_event_base_t base, int32_t id, void * event_data)
// {
//     if(base == M5STICKC_BUTTON_A_EVENT_BASE || base == M5STICKC_BUTTON_B_EVENT_BASE) {
//         M5StickCDisplayWakeup();
//     }
// }