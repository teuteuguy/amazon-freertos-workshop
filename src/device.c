/**
 * @file device.c
 * @brief Device specific code.
 *
 * (C) 2019 - Timothee Cruse <timothee.cruse@gmail.com>
 * This code is licensed under the MIT License.
 */

#include "device.h"

#include "freertos/FreeRTOS.h"

#include "m5stickc_display.h"
#include "esp_log.h"

#include "addons.h"


/*-----------------------------------------------------------*/

static const char *TAG = "device";

esp_event_loop_handle_t device_event_loop;

/*-----------------------------------------------------------*/

device_err_t prvDeviceInitCorePeripherals( device_t * config );

esp_err_t prvDeviceButtonEnable( device_button_t * button );
esp_err_t prvDeviceButtonDisable( device_button_t * button );
esp_err_t prvDeviceButtonSetAsInput( device_button_t * button );
esp_err_t prvDeviceButtonEnableInterrupt( device_button_t * button );
esp_err_t prvDeviceButtonDisableInterrupt( device_button_t * button );
bool prvDeviceButtonIsPressed( device_button_t * button );
void prvDeviceButtonTask( void * pvParameter );

/*-----------------------------------------------------------*/

void IRAM_ATTR prvDeviceButtonISRHandler( void* arg )
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    device_button_t * button = (device_button_t *) arg;

    if(gpio_get_level( button->gpio ) == 0) {
        xEventGroupSetBitsFromISR( button->event_group, DEVICE_BUTTON_PUSH_BIT, &xHigherPriorityTaskWoken );
    } else {
        xEventGroupSetBitsFromISR( button->event_group, DEVICE_BUTTON_POP_BIT, &xHigherPriorityTaskWoken );
    }
}

/*-----------------------------------------------------------*/

#if defined(DEVICE_ESP32_DEVKITC)

    ESP_EVENT_DEFINE_BASE( BUTTON_MAIN_EVENT_BASE )    /*!< BASE event of button A */
    device_button_t device_button_a = {
        .gpio =             DEVICE_HAS_MAIN_BUTTON,
        .debounce_time =    DEVICE_BUTTON_DEBOUNCE_TIME,
        .hold_time =        DEVICE_BUTTON_HOLD_TIME
    };

#elif defined(DEVICE_M5STICKC)

    ESP_EVENT_DEFINE_BASE( BUTTON_MAIN_EVENT_BASE )    /*!< BASE event of button A */
    ESP_EVENT_DEFINE_BASE( BUTTON_RESET_EVENT_BASE )   /*!< BASE event of button B */

    device_button_t device_button_a = {
        .gpio =             DEVICE_HAS_MAIN_BUTTON,
        .debounce_time =    DEVICE_BUTTON_DEBOUNCE_TIME,
        .hold_time =        DEVICE_BUTTON_HOLD_TIME
    };

    device_button_t device_button_b = {
        .gpio =             DEVICE_HAS_RESET_BUTTON,
        .debounce_time =    DEVICE_BUTTON_DEBOUNCE_TIME,
        .hold_time =        DEVICE_BUTTON_HOLD_TIME
    };


    uint8_t axp192_init_regs[] = {
        // Enable LDO2 & LDO3, LED & TFT 3.3V
        // 0x28 LDO2/3 Output Voltage Setting
        // BIT7-4: 1.8-3.3V, 100mV/step
        // BIT3-0: 1.8-3.3V, 100mV/step
        AXP192_REG_LDO2_LDO3_VOLTAGE_SETTING, (CONFIG_AXP192_28H_BIT7_4 << 4) | CONFIG_AXP192_28H_BIT3_0, // Set LDO2 & LDO3(TFT_LED & TFT) 3.0V

        // ADC setting the sample rate, TS pin control
        // BIT7: ADC Setting the sample rate Bit 1, 25 × 2 n
        // BIT6: ADC Setting the sample rate Bit 0, Sampling rates were 25 , 50 , 100 , 200Hz
        // BIT5-4: TS Output current setting pin: 00: 20uA ; 01: 40uA ; 10: 60uA ; 11: 80uA
        // BIT3: Reservations can not be changed
        // BIT2: TS Pin function selection, 0: Battery temperature monitoring function, 1: External independent ADC Input path
        // BIT1-0: TS Current output pin disposed, 00: shut down, 01: Output current charging, 10: ADC Input samples, can power 11: Has been opened
        AXP192_REG_ADC_SAMPLE_RATE_TS_PIN_CONTROL, (CONFIG_AXP192_84H_BIT7 << 7) | (CONFIG_AXP192_84H_BIT6 << 6) | (CONFIG_AXP192_84H_BIT5_4 << 4) | (CONFIG_AXP192_84H_BIT3 << 3) | (CONFIG_AXP192_84H_BIT2 << 2) | CONFIG_AXP192_84H_BIT1_0, // Set ADC sample rate to 200hz

        // Enable ADCs
        // 0x82 ADC Enable 1. 0 shut down. 1 turn on.
        // BIT7: Battery voltage ADC enable
        // BIT6: Battery current ADC enable
        // BIT5: ACIN voltage ADC enable
        // BIT4: ACIN electric current ADC enable
        // BIT3: VBUS voltage ADC enable
        // BIT2: VBUS electric current ADC enable
        // BIT1: APS voltage ADC enable
        // BIT0: TS pin ADC enable function
        AXP192_REG_ADC_ENABLE_1, (CONFIG_AXP192_82H_BIT7 << 7) | (CONFIG_AXP192_82H_BIT6 << 6) | (CONFIG_AXP192_82H_BIT5 << 5) | (CONFIG_AXP192_82H_BIT4 << 4) | (CONFIG_AXP192_82H_BIT3 << 3) | (CONFIG_AXP192_82H_BIT2 << 2) | (CONFIG_AXP192_82H_BIT1 << 1) | CONFIG_AXP192_82H_BIT0, // Set ADC to All Enable

        // Enable Charging
        // 0x33 Charging Control 1
        // BIT7: Enable control bit, outer and inner channel.
        // BIT6-5: Target voltage. 00 4.1V, 01 4.15V, 10 4.2V, 11 4.36V
        // BIT4: Current setting at the end of charge. 0 current is less than 10% whem the end of charging set value. 1 15%.
        // BIT 3-0: Current setting internal passage.   0000 100mA, 0001 190mA, 0010 280mA, 0011 360mA
        //                                              0100 450mA, 0101 550mA, 0110 630mA, 0111 700mA
        //                                              1000 780mA, 1001 880mA, 1010 960mA, 1011 1000mA
        //                                              1100 1080mA, 1101 1160mA, 1110 1240mA, 1111 1320mA
        AXP192_REG_CHARGE_CONTROL_1, (CONFIG_AXP192_33H_BIT7 << 7) | (CONFIG_AXP192_33H_BIT6_5 << 5) | (CONFIG_AXP192_33H_BIT4 << 4) | CONFIG_AXP192_33H_BIT3_0, // Bat charge voltage to 4.2, Current 100MA

        // Enable DC-DC1, OLED_VDD, 5B V_EXT
        // 0x12 Power supply output control
        // BIT6: EXTEN switch control
        // BIT4: DC-DC2 switch control
        // BIT3: LDO3 switch control
        // BIT2: LDO2 switch control // Handles M5StickC LCD Backlight
        // BIT1: DC-DC3 switch control
        // BIT0: DC-DC1 switch control
        AXP192_REG_DCDC1_DCDC3_LDO2_LDO3_SWITCH_CONTROL, (CONFIG_AXP192_12H_BIT6 << 6) | (CONFIG_AXP192_12H_BIT4 << 4) |(CONFIG_AXP192_12H_BIT3 << 3) | (CONFIG_AXP192_12H_BIT2 << 2) | (CONFIG_AXP192_12H_BIT1 << 1) | CONFIG_AXP192_12H_BIT0, // Depending on configuration enable LDO2, LDO3, DCDC1, DCDC3.
        // TODO: potentially check if this matches following: 
        // byte buf = (Read8bit(0x12) & 0xef) | 0x4D;
        // if(disableLDO3) buf &= ~(1<<3);
        // if(disableLDO2) buf &= ~(1<<2);
        // if(disableDCDC3) buf &= ~(1<<1);
        // if(disableDCDC1) buf &= ~(1<<0);
        // Write1Byte(0x12, buf);

        // Enable PEK
        // 0x36 PEK key parameters
        // BIT7-6: boot time settings. 00 128ms, 01 512ms, 10 1s, 11 2s.
        // BIT5-4: long time setting key. 00 1s, 01 1.5s, 10 2s, 11 2.5s.
        // BIT3: Automatic shutdown function ???
        // BIT2: PWROK signal delay after power-up complete. 0 32ms, 1 64ms.
        // BIT1-0: Long set off. 00 4s, 01 6s, 10 8s, 11 10s.
        AXP192_REG_PEK_SETTING, (CONFIG_AXP192_36H_BIT7_6 << 6) | (CONFIG_AXP192_36H_BIT5_4 << 4) | (CONFIG_AXP192_36H_BIT3 << 3) | (CONFIG_AXP192_36H_BIT2 << 2) | CONFIG_AXP192_36H_BIT1_0, // 128ms power on, 4s power off

        // GPIO0 Voltage output mode
        // BIT7-4: GPIO0 LDO Output voltage setting mode
        //         0000: 1.8V ; 0001: 1.9V ; 0010: 2.0V ; 0011: 2.1V ;
        //         0100: 2.2V ; 0101: 2.3V ; 0110: 2.4V ; 0111: 2.5V ;
        //         1000: 2.6V ; 1001: 2.7V ; 1010: 2.8V ; 1011: 2.9V ;
        //         1100: 3.0V ; 1101: 3.1V ; 1110: 3.2V ; 1111: 3.3V
        AXP192_REG_GPIO0_LDO_MODE_OUTPUT_VOLTAGE, (CONFIG_AXP192_91H_BIT7_4 << 4), // Set RTC voltage to 3.3V

        // Enable GPIO0
        // 0x90 GPIO0 feature
        // BIT2-0: 000 NMOS Open-drain output, 001 Universal input function, 010 Low noise LDO, 011 Retention.
        //         100 ADC entry, 101 output low, 11x floating
        AXP192_REG_GPIO0_CONTROL, CONFIG_AXP192_90H_BIT2_0, // Set GPIO0 to LDO

        // Enable USB thru mode
        // 0x30 VBUS-IPSOUT path management
        // BIT7: 0 N_VBUSEN pin selection. 1 VBUS-IPSOUT input selection regardles of N_VBUSEN status.
        // BIT6: VBUS Vhold limiting control. 0 no limit. 1 limit.
        // BIT5-3: Vhold. 000 4.0V, 001 4.1V, 010 4.2V, 011 4.3V, 100 4.4V, 101 4.5V, 110 4.6V, 111 4.7V.
        // BIT1: VBUS limiting control enable. 0 shutdown. 1 enable.
        // BIT0: VBUS limit control current. 0 500mA, 1 100mA.
        AXP192_REG_VBUS_IPSOUT_PATH_SETTING, (CONFIG_AXP192_30H_BIT7 << 7) | (CONFIG_AXP192_30H_BIT6 << 6) | (CONFIG_AXP192_30H_BIT5_3 << 3) | (CONFIG_AXP192_30H_BIT1 << 1) | CONFIG_AXP192_30H_BIT0, // Disable vbus hold limit

        // Battery charging High Temperature alarm
        // 0x39 VHTF-CHARGE: Default: 0x1F
        // BIT7-0: When charging the battery temperature threshold setting, N
        //         N * 10H , when N = 1FH ,correspond 0.397V ; Voltage may correspond to 0V ~ 3.264V
        //
        AXP192_REG_BATTERY_CHARGING_HIGH_TEMPERATURE_ALARM, CONFIG_AXP192_39H_BIT7_0, // Set temperature protection

        // Backup battery charging control
        // 0x35 BACKUP_BATTERY_CHARGING_CONTROL_REG Default: 0x22
        // BIT7: A spare battery enable control; 0: shut down; 1: turn on.
        // BIT6-5: Backup battery charging target voltage setting; 00: 3.1V ; 01: 3.0V ; 10: 3.0V ; 11: 2.5V.
        // BIT4-2: Reservations can not be changed
        // BIT1-0: Backup battery charging current setting: 00: 50uA ; 01: 100uA ; 10: 200uA ; 11: 400uA
        AXP192_REG_BACKUP_BATTERY_CHARGING_CONTROL, (CONFIG_AXP192_35H_BIT7 << 7) | (CONFIG_AXP192_35H_BIT6_5 << 5) | CONFIG_AXP192_35H_BIT1_0, // Enable RTC BAT charge 
        // TODO: manage disabling RTC Write1Byte(0x35, 0xa2 & (disableRTC ? 0x7F : 0xFF));

        // Set off, and the battery detection control pin CHGLED
        // 0x32 Default: 0x46
        // BIT7: the way A Shutdown control, This bit 1 Closes AXP192 Output
        // BIT6: Battery monitoring Set bit: 0: shut down; 1: turn on
        // BIT5-4: CHGLED Pin feature set, 00: High resistance, 01: 25% 1Hz flicker
        //                                 10: 25% 4Hz flicker, 11: Output low
        // BIT3: CHGLED Pin control settings, 0: Controlled by the charging function
        //                                    1: From the register REG 32HBit [5: 4] control
        // BIT1-0: N_OE After the low to high AXP192 Shutdown delay Delay time, 00: 0.5S ; 01: 1S ; 10: 2S ; 11: 3S
        AXP192_REG_OFF_BATTERY_DETECTION_CHGLED_CONTROL, (CONFIG_AXP192_32H_BIT7 << 7) | (CONFIG_AXP192_32H_BIT6 << 6) | (CONFIG_AXP192_32H_BIT5_4 << 4) | (CONFIG_AXP192_32H_BIT3 << 3) | (CONFIG_AXP192_32H_BIT2 << 2) | CONFIG_AXP192_32H_BIT1_0, // Enable bat detection

        // Enable 3.0V ??? What is the Voff function????
        // 0x31 Voff voltage setting
        // BIT3: Sleep mode PWRON press wakeup enable settings. 0 short press to wake up  ?????
        // BIT2-0: Voff setup. 000 2.6V, 001 2.7V, 010 2.8V, 011 2.9V, 100 3.0V, 101 3.1V, 110 3.2V, 111 3.3V
        AXP192_REG_VOFF_SHUTDOWN_VOLTAGE_SETTING, (CONFIG_AXP192_31H_BIT3 << 3) | CONFIG_AXP192_31H_BIT2_0, // Set Power off voltage 3.0v

        // OLED_VPP enable
        // 0x10 EXTEN & DC-DC2 output control
        // BIT2: EXTEN Switch control. 0 shut down. 1 turn on.
        // BIT0: DC-DC2 Switch control. 0 shut down. 1 turn on.
        AXP192_REG_EXTEN_DCDC2_SWITCH_CONTROL, (CONFIG_AXP192_10H_BIT2 << 2) | CONFIG_AXP192_10H_BIT0,

        // Enable Coulomb counter
        // 0xB8 Coulomb gauge control
        // BIT7: Switching control coulomb meter
        // BIT6: meter pause control. 1 pause metering. 0 resume.
        // BIT5: clear measurement
        AXP192_REG_COULOMB_COUNTER_CONTROL, (CONFIG_AXP192_B8H_BIT7 << 7) | (CONFIG_AXP192_B8H_BIT6 << 6) | (CONFIG_AXP192_B8H_BIT5 << 5)    
    };

    // 3 | ( DEVICE_ENABLE_BACKLIGHT_ON_BOOT == true ) ? (0x80 | (DEVICE_BACKLIGHT_SETTING_ON_BOOT << 4)) | 0x0F : 0x0
    // 11  | ( DEVICE_ENABLE_BACKLIGHT_ON_BOOT == true ) ? BIT2 : 0x0

    axp192_config_t axp192_config = {
        .i2c_handle     = NULL,
        .nb_init_regs   = 15,
        .init_regs      = axp192_init_regs
    };

    device_err_t prvDeviceM5StickCDisplayOff( void );
    device_err_t prvDeviceM5StickCDisplayOn( void );
    device_err_t prvDeviceM5StickCDisplayBacklightLevel( uint8_t level );

#endif

#if defined(DEVICE_HAS_ACCELEROMETER)
    static TaskHandle_t xAccelerometerTaskHandle;
    static void prvAccelerometerTask( void *pvParameters );
#endif // defined(DEVICE_HAS_ACCELEROMETER)

#if defined(ADDON_BMP280)
    static TaskHandle_t xBMP280TaskHandle;
    static void prvBMP280Task( void *pvParameters );
#endif // defined(DEVICE_HAS_ACCELEROMETER)

#if defined(DEVICE_HAS_BATTERY)
    static TaskHandle_t xBatteryTaskHandle;
    static void prvBatteryTask( void *pvParameters );
#endif // defined(DEVICE_HAS_BATTERY)

/*-----------------------------------------------------------*/

device_err_t eDeviceInit( device_t * config )
{
    if ( prvDeviceInitCorePeripherals( config ) != DEVICE_SUCCESS )
    {
        ESP_LOGE( TAG, "eDeviceInit: prvDeviceInitCorePeripherals failed" );
        return DEVICE_FAIL;
    }

    esp_event_loop_args_t loop_args = {
        .queue_size = 5,
        .task_name = "device_event_loop",
        .task_priority = 10,
        .task_stack_size = 2048,
        .task_core_id = 0
    };

    esp_err_t e = esp_event_loop_create( &loop_args, &device_event_loop );
    if ( e != ESP_OK )
    {
        ESP_LOGE( TAG, "eDeviceInit: Error creating event loop: %s", esp_err_to_name(e) );
        return DEVICE_FAIL;
    }

    #if defined(DEVICE_ESP32_DEVKITC)

        ESP_LOGI( TAG, "eDeviceInit: ESP32 DEVKITC" );
        // if ( eESP32DevkitcInit != ESP_OK )
        // {
        //     ESP_LOGE( TAG, "eDeviceInit: ESP32 DevkitC Init ... failed" );
        //     return DEVICE_FAIL;
        // }

        device_button_a.esp_event_base = BUTTON_MAIN_EVENT_BASE;

        #define ESP_INTR_FLAG_DEFAULT 0
        e = gpio_install_isr_service( ESP_INTR_FLAG_DEFAULT );
        if ( e == ESP_ERR_INVALID_STATE )
        {
            ESP_LOGD(TAG, "eDeviceInit: Button ISR service already installed");
        }
        else if ( e == ESP_ERR_NO_MEM || e == ESP_ERR_NOT_FOUND )
        {
            ESP_LOGE(TAG, "eDeviceInit: Error installing Button ISR service");
            return DEVICE_FAIL;
        }

        e = prvDeviceButtonEnable( &device_button_a );
        if(e == ESP_OK) {
            ESP_LOGD(TAG, "eDeviceInit: Button A enabled");
        } else {
            ESP_LOGE(TAG, "eDeviceInit: Error enabling button A");
            return DEVICE_FAIL;
        }

    #elif defined(DEVICE_M5STICKC)

        ESP_LOGI( TAG, "eDeviceInit: M5STICKC" );

        axp192_config.i2c_handle = config->i2c_handler[ AXP192_IRC_NUM ];        

        if ( eAXP192Init( axp192_config ) != AXP192_SUCCESS )
        {
            ESP_LOGE( TAG, "eDeviceInit: eAXP192Init failed" );
            return DEVICE_FAIL;
        }
        ESP_LOGI( TAG, "eDeviceInit: eAXP192Init ... Success!" );        

        device_button_a.esp_event_base = BUTTON_MAIN_EVENT_BASE;
        device_button_b.esp_event_base = BUTTON_RESET_EVENT_BASE;

        #define ESP_INTR_FLAG_DEFAULT 0
        e = gpio_install_isr_service( ESP_INTR_FLAG_DEFAULT );
        if ( e == ESP_ERR_INVALID_STATE )
        {
            ESP_LOGD(TAG, "eDeviceInit: Button ISR service already installed");
        }
        else if ( e == ESP_ERR_NO_MEM || e == ESP_ERR_NOT_FOUND )
        {
            ESP_LOGE(TAG, "eDeviceInit: Error installing Button ISR service");
            return DEVICE_FAIL;
        }

        e = prvDeviceButtonEnable( &device_button_a );
        if(e == ESP_OK) {
            ESP_LOGD(TAG, "eDeviceInit: Button A enabled");
        } else {
            ESP_LOGE(TAG, "eDeviceInit: Error enabling button A");
            return DEVICE_FAIL;
        }

        e += prvDeviceButtonEnable( &device_button_b ); // Notice += on error return to accumulate previous error
        if(e == ESP_OK) {
            ESP_LOGD(TAG, "eDeviceInit: Button B enabled");
        } else {
            ESP_LOGE(TAG, "eDeviceInit: Error enabling button B");
            return DEVICE_FAIL;
        }

        if ( M5StickCDisplayInit() != ESP_OK )
        {
            ESP_LOGE(TAG, "eDeviceInit: Error Initializing display");
            return DEVICE_FAIL;
        }

        if ( eDeviceDisplayInit() != DEVICE_SUCCESS )
        {
            ESP_LOGE(TAG, "eDeviceInit: Error Initializing display for device");
            return DEVICE_FAIL;
        }

    #endif // device type

    #if defined(ADDON_MPU6886)
        if ( eAddonMPU6886Init( config->i2c_handler[ ADDON_MPU6886 ] ) != ADDONS_SUCCESS )
        {
            ESP_LOGE( TAG, "eDeviceInit: Initialisation of MPU6886 addon ... failed" );
            return DEVICE_FAIL;
        }
        ESP_LOGI( TAG, "eDeviceInit: eAddonMPU6886Init ... Success!" );
    #endif

    #if defined(ADDON_BMP280)
        if ( eAddonBmp280Init( config->i2c_handler[ ADDON_BMP280 ] ) != ADDONS_SUCCESS )
        {
            ESP_LOGE( TAG, "eWorkshopInit: Initialisation of BMP280 addon ... failed" );
            return DEVICE_FAIL;
        }
        else
        {
            ESP_LOGI( TAG, "eDeviceInit: eAddonBmp280Init ... Success!" );
            /* Create Accelerometer reading task. */
            if ( !xTaskCreate( prvBMP280Task,		        /* The function that implements the task. */
                            "BMP280Task",    				/* The text name assigned to the task - for debug only as it is not used by the kernel. */
                            2048,		                    /* The size of the stack to allocate to the task. */
                            NULL,                           /* The parameter passed to the task - in this case the counter to increment. */
                            0,				                /* The priority assigned to the task. */
                            &xBMP280TaskHandle )	        /* The task handle is used to obtain the name of the task. */
                            )
            {
                ESP_LOGI( TAG, "eDeviceInit: Creation of the BMP280 task ... failed" );
            }
            ESP_LOGI( TAG, "eDeviceInit: Creation of the BMP280 task ... Success!" );
        }
        
    #endif

    #if defined(DEVICE_HAS_ACCELEROMETER)
        /* Create Accelerometer reading task. */
        if ( !xTaskCreate( prvAccelerometerTask,		/* The function that implements the task. */
                        "AccelTask",    				/* The text name assigned to the task - for debug only as it is not used by the kernel. */
                        2048,		                    /* The size of the stack to allocate to the task. */
                        NULL,                           /* The parameter passed to the task - in this case the counter to increment. */
                        0,				                /* The priority assigned to the task. */
                        &xAccelerometerTaskHandle )	    /* The task handle is used to obtain the name of the task. */
                        )
        {
            ESP_LOGI( TAG, "eDeviceInit: Creation of the Accelerometer task ... failed" );
        }
        ESP_LOGI( TAG, "eDeviceInit: Creation of the Accelerometer task ... Success!" );
    #endif // defined(DEVICE_HAS_ACCELEROMETER)

    #if defined(DEVICE_HAS_BATTERY)

        /* Create Battery reading task. */
        if ( !xTaskCreate( prvBatteryTask,			    /* The function that implements the task. */
                        "BatteryTask",    				/* The text name assigned to the task - for debug only as it is not used by the kernel. */
                        2048,		                    /* The size of the stack to allocate to the task. */
                        NULL,                           /* The parameter passed to the task - in this case the counter to increment. */
                        0,				                /* The priority assigned to the task. */
                        &xBatteryTaskHandle )	        /* The task handle is used to obtain the name of the task. */
                        )
        {
            ESP_LOGI( TAG, "eDeviceInit: Creation of the Battery task ... failed" );
        }
        ESP_LOGI( TAG, "eDeviceInit: Creation of the Battery task ... Success!" );

    #endif // defined(DEVICE_HAS_BATTERY)

    #if defined(DEVICE_HAS_STATUS_LED)
        gpio_config_t io_conf;
        // Setup the LED
        io_conf.intr_type = GPIO_PIN_INTR_DISABLE;                  //disable interrupt
        io_conf.mode = GPIO_MODE_OUTPUT;                            //set as output mode
        io_conf.pin_bit_mask = ((1ULL << DEVICE_HAS_STATUS_LED));   // bit mask of the pins that you want to set, e.g.GPIO10
        io_conf.pull_down_en = 0;                                   //disable pull-down mode
        io_conf.pull_up_en = 0;                                     //disable pull-up mode
        if ( gpio_config( &io_conf ) != ESP_OK ) //configure GPIO with the given settings
        {
            ESP_LOGE(TAG, "eDeviceInit: Error setting up Status LED" );
            return DEVICE_FAIL;
        }
        if ( DEVICE_STATUS_LED_OFF() != DEVICE_SUCCESS )
        {
            return DEVICE_FAIL;
        }
    #endif // defined(DEVICE_HAS_STATUS_LED)

    return DEVICE_SUCCESS;
}

/*-----------------------------------------------------------*/

device_err_t prvDeviceInitCorePeripherals( device_t * config )
{
    IotI2CConfig_t xI2CConfig =
    {
        .ulBusFreq       = IOT_I2C_FAST_MODE_BPS,
        .ulMasterTimeout = 500
    };
    
    // Open one of the I2C instance and get a handle.
    config->i2c_handler[ I2C_NUM_0 ] = iot_i2c_open( I2C_NUM_0 );
    if ( config->i2c_handler[ I2C_NUM_0 ] )
    {
        // Set I2C configuration.
        if ( iot_i2c_ioctl( config->i2c_handler[ I2C_NUM_0 ], eI2CSetMasterConfig, &xI2CConfig ) != IOT_I2C_SUCCESS )
        {
            ESP_LOGE( TAG, "prvDeviceInitCorePeripherals: iot_i2c_ioctl failed for %u", I2C_NUM_0 );
            return DEVICE_FAIL;
        }
    }
    else
    {
        ESP_LOGE( TAG, "prvDeviceInitCorePeripherals: iot_i2c_open %u failed", I2C_NUM_0 );
        return DEVICE_FAIL;
    }

    // Open one of the I2C instance and get a handle.
    config->i2c_handler[ I2C_NUM_1 ] = iot_i2c_open( I2C_NUM_1 );
    if ( config->i2c_handler[ I2C_NUM_1 ] )
    {
        // Set I2C configuration.
        if ( iot_i2c_ioctl( config->i2c_handler[ I2C_NUM_1 ], eI2CSetMasterConfig, &xI2CConfig ) != IOT_I2C_SUCCESS )
        {
            ESP_LOGE( TAG, "prvDeviceInitCorePeripherals: iot_i2c_ioctl failed for %u", I2C_NUM_1 );
            return DEVICE_FAIL;
        }
    }
    else
    {
        ESP_LOGE( TAG, "prvDeviceInitCorePeripherals: iot_i2c_open %u failed", I2C_NUM_1 );
        return DEVICE_FAIL;
    }

    return DEVICE_SUCCESS;
}

/*-----------------------------------------------------------*/

esp_err_t eDeviceRegisterButtonCallback(esp_event_base_t base, void (*callback)(void * handler_arg, esp_event_base_t base, int32_t id, void * event_data) )
{
    esp_err_t res = ESP_FAIL;
    if (device_event_loop)
    {
        res = esp_event_handler_register_with(device_event_loop, base, ESP_EVENT_ANY_ID, callback, NULL);
        ESP_LOGD(TAG, "eDeviceRegisterButtonCallback: Button registered... %s", res == ESP_OK ? "OK" : "NOK");
    }
    else
    {
        ESP_LOGE(TAG, "eDeviceRegisterButtonCallback: device_event_loop is NULL");
    }    
    return res;
}

/*-----------------------------------------------------------*/

#if defined(DEVICE_HAS_ACCELEROMETER)
    static void prvAccelerometerTask( void *pvParameters )
    {
        TickType_t xDelayTimeInTicks = pdMS_TO_TICKS( 1000 );        

        for( ;; )
        {
            #if defined(ADDON_MPU6886)
                addon_mpu6886_sensors_t xMPU6886SensorData;
                if ( eAddonMPU6886GetSensors( &xMPU6886SensorData ) == ADDONS_SUCCESS )
                {
                    ESP_LOGD( TAG, "Device: Accel(%f, %f, %f)  Gyro(%f, %f, %f) Temp(%f) AHRS(%f, %f, %f)",
                            xMPU6886SensorData.accel_x, xMPU6886SensorData.accel_y, xMPU6886SensorData.accel_z,
                            xMPU6886SensorData.gyro_x, xMPU6886SensorData.gyro_y, xMPU6886SensorData.gyro_z,
                            xMPU6886SensorData.temperature,
                            xMPU6886SensorData.pitch, xMPU6886SensorData.roll, xMPU6886SensorData.yaw );
                }
            #endif
            
            vTaskDelay( xDelayTimeInTicks );
        }

        vTaskDelete( NULL );
    }
#endif // defined(DEVICE_HAS_ACCELEROMETER)

/*-----------------------------------------------------------*/

#if defined(ADDON_BMP280)
    static void prvBMP280Task( void *pvParameters )
    {
        TickType_t xDelayTimeInTicks = pdMS_TO_TICKS( 5000 );        

        for( ;; )
        {
            #if defined(ADDON_BMP280)
                addon_bmp280_sensors_t xBmp280SensorData;
                if ( eAddonBmp280GetSensors( &xBmp280SensorData, 100, 1010 ) == ADDONS_SUCCESS )
                {
                    ESP_LOGD( TAG, "Device: T: %fC, P: %fmBar, SeaLevel: %fmBar, Altitude: %fm", 
                            xBmp280SensorData.temperature,
                            xBmp280SensorData.pressure,
                            xBmp280SensorData.seaLevel,
                            xBmp280SensorData.altitude );
                }
            #endif

            vTaskDelay( xDelayTimeInTicks );
        }

        vTaskDelete( NULL );
    }
#endif // defined(ADDON_BMP280)

/*-----------------------------------------------------------*/

#if defined(DEVICE_HAS_BATTERY)
    static void prvBatteryTask( void *pvParameters )
    {
        TickType_t xDelayTimeInTicks = pdMS_TO_TICKS( 10000 );

        for( ;; )
        {

            #if defined(DEVICE_M5STICKC)

                int status = EXIT_SUCCESS;
                uint16_t vbat = 0, vaps = 0, b = 0, c = 0, battery = 0;
                char pVbatStr[11] = {0};

                if ( eAXP192GetVbat( &vbat ) != AXP192_SUCCESS || eAXP192GetVaps( &vaps ) != AXP192_SUCCESS )
                {
                    ESP_LOGE( TAG, "prvBatteryTask: Failed to get Vbat and Vaps" );
                }
                else 
                {
                    ESP_LOGD(TAG, "prvBatteryTask: VBat:         %u", vbat);
                    ESP_LOGD(TAG, "prvBatteryTask: VAps:         %u", vaps);
                    b = (vbat * 1.1);
                    ESP_LOGD(TAG, "prvBatteryTask: b:            %u", b);
                    c = (vaps * 1.4);
                    ESP_LOGD(TAG, "prvBatteryTask: c:            %u", c);
                    battery = ((b - 3000)) / 12;
                    ESP_LOGD(TAG, "prvBatteryTask: battery:      %u", battery);

                    if (battery >= 100)
                    {
                        battery = 99; // No need to support 100% :)
                    }

                    if (c >= 4500) //4.5)
                    {
                        status = snprintf(pVbatStr, 11, "CHG: %02u%%", battery);
                    }
                    else
                    {
                        status = snprintf(pVbatStr, 11, "BAT: %02u%%", battery);
                    }

                    if (status < 0) {
                        ESP_LOGE(TAG, "prvBatteryTask: error with creating battery string");
                    }
                    else
                    {
                        ESP_LOGD(TAG, "prvBatteryTask: Charging str(%i): \"%s\"", status, pVbatStr);
                        TFT_print(pVbatStr, 1, M5STICKC_DISPLAY_HEIGHT - 13);
                    }
                }

            #endif

            vTaskDelay( xDelayTimeInTicks );
        }

        vTaskDelete( NULL );
    }
#endif

/*-----------------------------------------------------------*/

#if defined(DEVICE_M5STICKC)

    device_err_t eDeviceDisplayInit( void )
    {
        TFT_FONT_ROTATE = 0;
        TFT_TEXT_WRAP = 0;
        TFT_FONT_TRANSPARENT = 0;
        TFT_FONT_FORCEFIXED = 0;
        TFT_GRAY_SCALE = 0;
        TFT_setGammaCurve(DEFAULT_GAMMA_CURVE);
        TFT_setRotation(LANDSCAPE_FLIP);
        TFT_setFont(DEFAULT_FONT, NULL);
        TFT_resetclipwin();
        TFT_fillScreen(TFT_BLACK);
        TFT_FONT_BACKGROUND = TFT_BLACK;
        TFT_FONT_FOREGROUND = TFT_ORANGE;

        if ( prvDeviceM5StickCDisplayOn() == DEVICE_SUCCESS && prvDeviceM5StickCDisplayBacklightLevel( 7 ) == DEVICE_SUCCESS )
        {
            #define SCREEN_OFFSET 2
            #define SCREEN_LINE_HEIGHT 14
            #define SCREEN_LINE_1  SCREEN_OFFSET + 0 * SCREEN_LINE_HEIGHT
            #define SCREEN_LINE_2  SCREEN_OFFSET + 1 * SCREEN_LINE_HEIGHT
            #define SCREEN_LINE_3  SCREEN_OFFSET + 2 * SCREEN_LINE_HEIGHT
            #define SCREEN_LINE_4  SCREEN_OFFSET + 3 * SCREEN_LINE_HEIGHT

            TFT_print((char *)"AWS & FreeRTOS", CENTER, SCREEN_LINE_1);
            TFT_print((char *)"workshop", CENTER, SCREEN_LINE_2);

            #if defined(LABCONFIG_LAB0_DO_NOTHING)
                TFT_print((char *)"LAB0 - DOES NOTHING", CENTER, SCREEN_LINE_4);
                ESP_LOGI( TAG, "eDeviceDisplayInit: LAB0 - DOES NOTHING" );
            #elif defined(LABCONFIG_LAB1_AWS_IOT_BUTTON)
                TFT_print((char *)"LAB1 - AWS IOT BUTTON", CENTER, SCREEN_LINE_4);
                ESP_LOGI( TAG, "eDeviceDisplayInit: LAB1 - AWS IOT BUTTON" );
            #elif defined(LABCONFIG_LAB2_SHADOW)
                TFT_print((char *)"LAB2 - THING SHADOW", CENTER, SCREEN_LINE_4);
                ESP_LOGI( TAG, "eDeviceDisplayInit: LAB2 - THING SHADOW" );
            #endif

            TFT_drawLine(0, M5STICKC_DISPLAY_HEIGHT - 13 - 3, M5STICKC_DISPLAY_WIDTH, M5STICKC_DISPLAY_HEIGHT - 13 - 3, TFT_ORANGE);

            return DEVICE_SUCCESS;
        }

        return DEVICE_FAIL;
    }

    device_err_t prvDeviceM5StickCDisplayOff( void )
    {
        if ( eAXP192ClearRegisterBits( AXP192_REG_DCDC1_DCDC3_LDO2_LDO3_SWITCH_CONTROL, BIT2 ) != AXP192_SUCCESS )
        {
            ESP_LOGE( TAG, "eAXP192ClearRegisterBits: Error turning display off" );
            return DEVICE_FAIL;
        }
        return DEVICE_SUCCESS;
    }

    device_err_t prvDeviceM5StickCDisplayOn( void )
    {
        if ( eAXP192SetRegisterBits( AXP192_REG_DCDC1_DCDC3_LDO2_LDO3_SWITCH_CONTROL, BIT2 ) != AXP192_SUCCESS )
        {
            ESP_LOGE( TAG, "eAXP192SetRegisterBits: Error turning display on" );
            return DEVICE_FAIL;
        }
        return DEVICE_SUCCESS;
    }

    device_err_t prvDeviceM5StickCDisplayBacklightLevel( uint8_t level )
    {
        if ( eAXP192SetRegisterBits( AXP192_REG_LDO2_LDO3_VOLTAGE_SETTING, (0x80 | (level << 4)) | 0x0F ) != AXP192_SUCCESS )
        {
            ESP_LOGE( TAG, "eAXP192SetRegisterBits: Error setting backlight level" );
            return DEVICE_FAIL;
        }
        return DEVICE_SUCCESS;
    }

#endif


/*-----------------------------------------------------------*/

device_err_t eDeviceSetLed( uint8_t port, uint32_t value )
{
    if ( gpio_set_level( port, value) != ESP_OK )
    {
        return DEVICE_FAIL;
    }
    return DEVICE_SUCCESS;
}

/*-----------------------------------------------------------*/

esp_err_t prvDeviceButtonEnable( device_button_t * button )
{
    esp_err_t e;

    if( button == NULL ) {
        return ESP_ERR_INVALID_ARG;
    }

    // Set gpio as input
    if ( prvDeviceButtonSetAsInput( button ) != ESP_OK )
    {
        return ESP_FAIL;
    }

    // Init event_group
    #if defined(CONFIG_SUPPORT_STATIC_ALLOCATION)
        button->event_group = xEventGroupCreateStatic( &(button->event_group_buffer) );
    #else
        button->event_group = xEventGroupCreate();
    #endif
    
    if ( button->event_group == NULL )
    {
        ESP_LOGE(TAG, "prvDeviceButtonEnable: Error creating button event group");
        return ESP_FAIL;
    }

    // Start task
    #if defined(CONFIG_SUPPORT_STATIC_ALLOCATION)
        button->task = xTaskCreateStatic(
                            prvDeviceButtonTask, 
                            "button_task", 
                            2048,
                            (void *) button,
                            20,
                            button->task_stack,
                            &(button->task_buffer)
                        );
        if ( button->task == NULL )
        {
            ESP_LOGE(TAG, "prvDeviceButtonEnable: Error creating button_task");
            vEventGroupDelete( button->event_group );
            return ESP_FAIL;
        }
    #else
        if( xTaskCreate(
                            prvDeviceButtonTask, 
                            "button_task",
                            2048,
                            (void *) button,
                            20,
                            &(button->task)
                        ) != pdPASS)
        {
            ESP_LOGE(TAG, "prvDeviceButtonEnable: Error creating button_task");
            vEventGroupDelete( button->event_group );
            return ESP_FAIL;
        }
    #endif

    // Set interrupt type
    e = gpio_set_intr_type( button->gpio, GPIO_INTR_ANYEDGE );
    if (e != ESP_OK ) {
        return e;
    }

    // Enable interrupt
    e = prvDeviceButtonEnableInterrupt(button);
    if (e != ESP_OK) {
        vTaskDelete( button->task );
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t prvDeviceButtonDisable( device_button_t * button )
{
    if ( button == NULL ) {
        return ESP_ERR_INVALID_ARG;
    }

    vEventGroupDelete(button->event_group);
    vTaskDelete(button->task);

    return ESP_OK;
}

esp_err_t prvDeviceButtonSetAsInput(device_button_t * button)
{
    esp_err_t e;

    gpio_pad_select_gpio(button->gpio);
    e = gpio_set_direction(button->gpio, GPIO_MODE_INPUT);
    if(e != ESP_OK) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t prvDeviceButtonEnableInterrupt(device_button_t * button)
{
    esp_err_t e;

    e = gpio_isr_handler_add(button->gpio, prvDeviceButtonISRHandler, button);
    if(e != ESP_OK) {
        return e;
    }

    return ESP_OK;
}

esp_err_t prvDeviceButtonDisableInterrupt(device_button_t * button)
{
    return gpio_isr_handler_remove(button->gpio);
}

bool prvDeviceButtonIsPressed(device_button_t * button)
{
    return (gpio_get_level(button->gpio) == 0) ? true : false;
}

void prvDeviceButtonTask(void * pvParameter)
{
    EventBits_t event;
    device_button_t * button = (device_button_t *) pvParameter;

    ESP_LOGD(TAG, "prvDeviceButtonTask: Button task started");

    while(1) {
        event = xEventGroupWaitBits(button->event_group, DEVICE_BUTTON_PUSH_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
        if((event & DEVICE_BUTTON_PUSH_BIT) != 0) {
            vTaskDelay(button->debounce_time/portTICK_PERIOD_MS);
            xEventGroupClearBits(button->event_group, DEVICE_BUTTON_POP_BIT);
            event = xEventGroupWaitBits(button->event_group, DEVICE_BUTTON_POP_BIT, pdTRUE, pdFALSE, button->hold_time / portTICK_PERIOD_MS);
            if((event & DEVICE_BUTTON_POP_BIT) != 0) {
                esp_event_post_to(device_event_loop, button->esp_event_base, DEVICE_BUTTON_CLICK_EVENT, NULL, 0, portMAX_DELAY);
                ESP_LOGD(TAG, "prvDeviceButtonTask: BUTTON_CLICK event");
            } else {
                esp_event_post_to(device_event_loop, button->esp_event_base, DEVICE_BUTTON_HOLD_EVENT, NULL, 0, portMAX_DELAY);
                ESP_LOGD(TAG, "prvDeviceButtonTask: BUTTON_HOLD event");
            }
        }
    }
}