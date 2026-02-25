
#include <stdio.h>
#include "pico/stdlib.h"
#include <stdint.h>
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "pico/binary_info.h"

#include "bmp280.h"
#include "bno085.h"
#include "sh2.h"
#include "gtu7.h"

char line[MINMEA_MAX_SENTENCE_LENGTH];

int main(void) {
    // Initialize UART for debugging output
    stdio_init_all();
    
    // Wait a moment for USB to initialize
    sleep_ms(10000);
    
    // init i2c for bmp280 and bno085
    i2c_init(BNO085_I2C, 100 * 1000);
    gpio_set_function(BNO085_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(BNO085_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(BNO085_SDA_PIN);
    gpio_pull_up(BNO085_SCL_PIN);
    printf("I2C initialized on pins SDA=%d, SCL=%d\n", PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN);

    // gps initialisation
    while(!gps_init(GTU7_UART, GTU7_TX, GTU7_RX, GTU7_BAUD)){
        sleep_ms(1000);
        printf("init failed, retrying...\n");
    }
    printf("gps init successful\n");
    
    // Reset the BMP280
    bmp280_reset();
    sleep_ms(1000);  //wait after reset
    printf("bmp280 reset\n");
    
    // Initialize the BMP280
    bmp280_init();
    sleep_ms(1000);  //wait after init
    printf("bmp280 initialised\n");
    
    // Read calibration parameters
    struct bmp280_calib_param calib_params;
    bmp280_get_calib_params(&calib_params);
    // print struct for debug/info
    printf("Calibration parameters loaded\n");
    printf("dig_t1=%u, dig_t2=%d, dig_t3=%d\n", calib_params.dig_t1, calib_params.dig_t2, calib_params.dig_t3);
    printf("dig_p1=%u, dig_p2=%d, dig_p3=%d\n", calib_params.dig_p1, calib_params.dig_p2, calib_params.dig_p3);
    
    // main loop
    while (1) {
        int32_t raw_temp, raw_pressure;
        bmp280_read_raw(&raw_temp, &raw_pressure);
        
        int32_t temp_c = bmp280_convert_temp(raw_temp, &calib_params);
        uint32_t pressure_pa = bmp280_convert_pressure(raw_pressure, raw_temp, &calib_params);
        
        float temperature = temp_c / 100.0f;
        
        printf("temp: %.3f pressure: %lu\n", temperature, pressure_pa);

        if (read_uart(line, MINMEA_MAX_SENTENCE_LENGTH) == true) {
            printf("nmea sentence: %s\n", line);
            // translate gps data
            gps_get_sentence(line);
            // copy data from gps_data into gps
            gps_data_t gps = gps_data();
            printf("UTC: %02d:%02d:%02d | Lat: %d, Lon: %d, Alt: %.2fm, Fix: %d\n", 
            gps.hour, gps.min, gps.sec, gps.lat, gps.lon, gps.alt, gps.fix_quality);
        } 
        sleep_ms(1);
    }
    
    return 0;
}
