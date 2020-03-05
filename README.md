# Amazon Freertos workshop code using the M5 StickC

Amazon FreeRTOS code using [M5StickC](https://docs.m5stack.com/#/en/core/m5stickc)


## Introduction

A collection of labs designed to run on the M5StickC device. Includes multiple labs to get up and running.

- Lab0: Init device. Does nothing really.
- Lab1: Create your own AWS IoT Button.
- Lab2: Remote control an Air Conditioning unit using AWS IoT Thing Shadows.
- Lab3: Alexa

## Start

The workshop documentation and content is located [here](https://teuteuguy.github.io/amazon-freertos-workshop-docs/)

## Run

Simply clone the repository.

Choose your lab by editing the `./include/lab_config.h` header file. Change your lab from one of the following choices

- LABCONFIG_LAB0_DO_NOTHING
- LABCONFIG_LAB1_AWS_IOT_BUTTON
- LABCONFIG_LAB2_SHADOW

Then run: 

```bash
cmake -S . -B build -DIDF_SDKCONFIG_DEFAULTS=./sdkconfig -DCMAKE_TOOLCHAIN_FILE=amazon-freertos/tools/cmake/toolchains/xtensa-esp32.cmake -GNinja

ESPPORT=[YOUR SERIALPORT] ESPBAUD=1500000 cmake --build build --target flash
```

Then monitor:
```bash
screen [YOUR SERIALPORT] 115200 -L
```

Or everything in one line: 
```bash
cmake -S . -B build -DIDF_SDKCONFIG_DEFAULTS=./sdkconfig -DCMAKE_TOOLCHAIN_FILE=amazon-freertos/tools/cmake/toolchains/xtensa-esp32.cmake -G
Ninja && ESPPORT=[YOUR SERIALPORT] ESPBAUD=1500000 cmake --build build --target flash && screen [YOUR SERIALPORT] 115200 -L
```

Note: the workshop currently supports 2 devices, ESP32 DevkitC and the M5StickC. Choose your device by editing the `./include/lab_config.h` header file.

# Disclaimer
The following workshop material including documentation and code, is provided as is. You may incur AWS service costs for using the different resources outlined in the labs. Material is provided AS IS and is to be used at your own discretion. The author will not be responsible for any issues you may run into by using this material. 

If you have any feedback, suggestions, please use the issues section.