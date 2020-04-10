cmake_minimum_required(VERSION 3.13)

project(afr_workshop)

file(GLOB SOURCES "src/*.c")
add_executable(afr_workshop ${SOURCES})

# Add workshop code and files.
include_directories(
    afr_workshop 
    PRIVATE 
        "include"
)

# Tell IDF build to link against this target.
set(IDF_PROJECT_EXECUTABLE afr_workshop)

# Add some extra components. IDF_EXTRA_COMPONENT_DIRS is an variable used by ESP-IDF to collect extra components.
get_filename_component( EXTRA_COMPONENT_DIRS "components/afr-esp32devkitc-bsp" ABSOLUTE )
list( APPEND IDF_EXTRA_COMPONENT_DIRS ${EXTRA_COMPONENT_DIRS} )
# get_filename_component( EXTRA_COMPONENT_DIRS "components/afr-m5stickc-bsp" ABSOLUTE )
# list( APPEND IDF_EXTRA_COMPONENT_DIRS ${EXTRA_COMPONENT_DIRS} )
get_filename_component( EXTRA_COMPONENT_DIRS "components/m5stickc-display" ABSOLUTE )
list( APPEND IDF_EXTRA_COMPONENT_DIRS ${EXTRA_COMPONENT_DIRS} )

# As of now there's no offical way to redefine config files outside of Amazon FreeRTOS source tree.
# This is a temporary approach to inject an include path so that this takes precedence over the
# config file directory inside Amazon FreeRTOS.
include_directories(BEFORE amazon-freertos-configs)

# Add amazon freertos as an subdirectory. AFR_BOARD tells which board to target.
set(AFR_BOARD espressif.esp32_devkitc CACHE INTERNAL "")
add_subdirectory(amazon-freertos)

message("")
# message("=============================== DEVICES =================================")
# add_subdirectory( "components/afr-m5stickc-bsp" )
# list( APPEND DEVICE_LIBS m5stickc )
message("=============================== SENSORS =================================")
# Demonstrate how to add a dummy module that depends on AFR based modules.
add_subdirectory( "components/foo" )
list( APPEND SENSOR_LIBS foo )
add_subdirectory( "components/afr-axp192" )
list( APPEND SENSOR_LIBS axp192 )
add_subdirectory( "components/afr-mpu6886" )
list( APPEND SENSOR_LIBS mpu6886 )
add_subdirectory( "components/afr-bmp280" )
list( APPEND SENSOR_LIBS bmp280 )
message("=========================================================================")

# Link against the mqtt demo so that we can use it. Dependencies of this demo are transitively
# linked.
target_link_libraries(
    afr_workshop 
    PRIVATE 
        AFR::common
        AFR::common_io
        AFR::demo_ble_numeric_comparison
        AFR::demo_mqtt
        AFR::demo_wifi_provisioning
        AFR::shadow
    PRIVATE
        # ${DEVICE_LIBS}
        ${SENSOR_LIBS}
)
