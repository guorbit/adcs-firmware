#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/i2c_slave.h"
#include "adcs_config.h"
#include "GPS.h"
#include "hardware/uart.h"
// Based on Raspberry Pi Pico SDK BMP280 example
// Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
// SPDX-License-Identifier: BSD-3-Clause


// main from bmp280 code,, :'D
int main() {
    stdio_init_all();

#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
    #warning i2c / bmp280_i2c example requires a board with I2C pins
        puts("Default I2C pins were not defined");
    return 0;
#else
    // useful information for picotool
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
    bi_decl(bi_program_description("BMP280 I2C example for the Raspberry Pi Pico"));

    printf("Hello, BMP280! Reading temperaure and pressure values from sensor...\n");

    // I2C is "open drain", pull ups to keep signal high when no data is being sent
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    // configure BMP280
    bmp280_init();

    // retrieve fixed compensation params
    struct bmp280_calib_param params;
    bmp280_get_calib_params(&params);

    int32_t raw_temperature;
    int32_t raw_pressure;

    sleep_ms(250); // sleep so that data polling and register update don't collide
    while (1) {
        bmp280_read_raw(&raw_temperature, &raw_pressure);
        int32_t temperature = bmp280_convert_temp(raw_temperature, &params);
        int32_t pressure = bmp280_convert_pressure(raw_pressure, raw_temperature, &params);
        printf("Pressure = %.3f kPa\n", pressure / 1000.f);
        printf("Temp. = %.2f C\n", temperature / 100.f);
        // poll every 500ms
        sleep_ms(500);
    }
#endif
}



//GPS Stuffs for when tidier

//put this in setup/nitialization

// uart0: UART peripheral used
// GPIO 0: uart TX
// GPIO 1: uart RX
//9600: baud rate
GPS::getInstance()->setUp(uart0, 0, 1, 9600);

// put this in main loop:

//prints GPS data as CSV: time, lat, lon, height
printf("%s,%.5f,%.5f,%.1f\n",
    GPS::getInstance()->getTime(),
    GPS::getInstance()->getLat(),
    GPS::getInstance()->getLon(),
    GPS::getInstance()->getHeight());
















//BMP preasure for when tidier

#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "bmp280.h"

int main() {
    // Initialize UART for debugging output
    stdio_init_all();
    
    // Wait a moment for USB to initialize
    sleep_ms(1000);
    
    // Initialize I2C port (using default pins)
    i2c_init(i2c_default, 400 * 1000);  // 400 kHz
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    
    printf("I2C initialized on pins SDA=%d, SCL=%d\n", PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN);
    
    // Test I2C communication - try to read from BMP280
    uint8_t test_buf[1];
    int ret = i2c_read_blocking(i2c_default, 0x76, test_buf, 1, false);
    printf("I2C read test (addr 0x76): %s\n", ret >= 0 ? "SUCCESS" : "FAILED");
    
    if (ret < 0) {
        printf("Trying alternate address 0x77...\n");
        ret = i2c_read_blocking(i2c_default, 0x77, test_buf, 1, false);
        printf("I2C read test (addr 0x77): %s\n", ret >= 0 ? "SUCCESS" : "FAILED");
    }
    
    // Reset the BMP280
    bmp280_reset();
    sleep_ms(100);  //wait after reset
    
    // Initialize the BMP280
    bmp280_init();
    sleep_ms(100);  //wait after init
    
    // Read calibration parameters
    struct bmp280_calib_param calib_params;
    bmp280_get_calib_params(&calib_params);
    printf("Calibration parameters loaded\n");
    printf("dig_t1=%u, dig_t2=%d, dig_t3=%d\n", calib_params.dig_t1, calib_params.dig_t2, calib_params.dig_t3);
    printf("dig_p1=%u, dig_p2=%d, dig_p3=%d\n", calib_params.dig_p1, calib_params.dig_p2, calib_params.dig_p3);
    
    while (1) {
        int32_t raw_temp, raw_pressure;
        bmp280_read_raw(&raw_temp, &raw_pressure);
        
        int32_t temp_c = bmp280_convert_temp(raw_temp, &calib_params);
        uint32_t pressure_pa = bmp280_convert_pressure(raw_pressure, raw_temp, &calib_params);
        
        float temperature = temp_c / 100.0f;
        
        printf("%.2f\t\t%lu\n", temperature, pressure_pa);
        
        sleep_ms(1000);
    }
    
    return 0;
}
