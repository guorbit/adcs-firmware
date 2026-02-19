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

add_compile_options(-Dtimegm=mktime)

set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(ADCS_git C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add GPS module - PUT IT HERE (after pico_sdk_init)
set(MINNMEA_PATH "${CMAKE_CURRENT_LIST_DIR}/lib")
include(minnmea.cmake)

# Add executable. Default name is the project name, version 0.1

add_executable(ADCS_git
    src/main.c
    src/gtu7.c
    src/bmp280.c
    src/bno085.c
    src/obc.c
    lib/minmea.c
    lib/sh2_SensorValue.c
    lib/sh2_util.c
    lib/sh2.c
    lib/shtp.c
)

pico_set_program_name(ADCS_git "ADCS_git")
pico_set_program_version(ADCS_git "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(ADCS_git 0)
pico_enable_stdio_usb(ADCS_git 1)

# Add the standard library to the build
target_link_libraries(ADCS_git
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(ADCS_git PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/lib
)

# Add any user requested libraries
target_link_libraries(ADCS_git 
        hardware_i2c
        pico_i2c_slave
        hardware_uart
        hardware_irq
        )

pico_add_extra_outputs(ADCS_git)