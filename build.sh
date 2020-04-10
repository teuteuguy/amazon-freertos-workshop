#!/bin/bash
cmake --clean-first -S . -B build -DIDF_SDKCONFIG_DEFAULTS=./sdkconfig -DCMAKE_TOOLCHAIN_FILE=amazon-freertos/tools/cmake/toolchains/xtensa-esp32.cmake -GNinja && ESPPORT=/dev/cu.usbserial-7952261DB4 ESPBAUD=1500000 cmake --build build --target flash && screen /dev/cu.usbserial-7952261DB4 115200 -L
