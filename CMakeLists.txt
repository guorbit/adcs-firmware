# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

add_compile_options(-Dtimegm=mktime)

set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(adcs-firmware C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()
add_definitions(-DPICO_I2C_DEFAULT_TIMEOUT_US=1000000)

# Add GPS module - PUT IT HERE (after pico_sdk_init)
set(MINNMEA_PATH "${CMAKE_CURRENT_LIST_DIR}/lib")
include(minnmea.cmake)

# Add executable. Default name is the project name, version 0.1

add_executable(adcs-firmware
    src/main.c
    src/gtu7.c
    src/bmp280.c
    src/bno085.c
    src/blink.c
    src/i2c_utils.c
    src/obc.c
    src/sensor_i2c.c
    lib/minmea.c
    lib/sh2_SensorValue.c
    lib/sh2_util.c
    lib/sh2.c
    lib/shtp.c
)

pico_set_program_name(adcs-firmware "adcs-firmware")
pico_set_program_version(adcs-firmware "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(adcs-firmware 0)
pico_enable_stdio_usb(adcs-firmware 1)

# Specify uart0 pins 16 and 17 as default
TARGET_COMPILE_DEFINITIONS(adcs-firmware PRIVATE 
    PICO_DEFAULT_UART_TX_PIN=16
    PICO_DEFAULT_UART_RX_PIN=17)


# Add any user requested libraries
target_link_libraries(adcs-firmware 
        pico_stdlib
        hardware_i2c
        pico_i2c_slave
        hardware_uart
        hardware_irq
        pico_multicore
        pico_sync
        )

# Add the standard include files to the build
target_include_directories(adcs-firmware PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/lib
)

pico_add_extra_outputs(adcs-firmware)