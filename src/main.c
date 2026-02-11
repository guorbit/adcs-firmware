#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "pico/binary_info.h"
#include "sh2.h"

// sensor headers
#include "bmp280.h"
#include "bno085.h"
#include "sh2.h"
#include "GPS.h"

// because the bno085 keeps browning out, i will add some capacitors
void i2c_bus_reset() {
    printf("Resetting I2C bus...\n");
    i2c_deinit(i2c_default);
    gpio_init(BNO085_SCL_PIN);
    gpio_init(BNO085_SDA_PIN);
    gpio_set_dir(BNO085_SCL_PIN, GPIO_OUT);
    for (int i = 0; i < 10; i++) {
        gpio_put(BNO085_SCL_PIN, 0); sleep_us(5);
        gpio_put(BNO085_SCL_PIN, 1); sleep_us(5);
    }
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(BNO085_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(BNO085_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(BNO085_SDA_PIN);
    gpio_pull_up(BNO085_SCL_PIN);
}

int main() {
    stdio_init_all();
    sleep_ms(3000); // Wait for USB serial
    printf("system boot\n");

    // initialize i2c (BMP280 and BNO085 share a bus)
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(BNO085_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(BNO085_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(BNO085_SDA_PIN);
    gpio_pull_up(BNO085_SCL_PIN);

    // initialize gps (uart)
    GPS_Init(uart0, 0, 1, 9600);

    // initialize BMP280
    bmp280_init();
    // defined in bmp280.h
    struct bmp280_calib_param bmp_params;
    bmp280_get_calib_params(&bmp_params);
    printf("BMP280 Initialized.\n");

    // initialize BNO085
    if (!bno085_init()) {
        printf("BNO085 Init Failed!\n");
    } else {
        sh2_devReset();
        sleep_ms(200);
        // Enable Reports
        enable_report(SH2_ROTATION_VECTOR, 10000);
        enable_report(SH2_ACCELEROMETER, 10000);
        enable_report(SH2_MAGNETIC_FIELD_CALIBRATED, 10000);
        printf("BNO085 Initialized.\n");
    }

    uint32_t last_print = 0;
    bno085_state_t bno_state;

    while (1) {
        // Poll Sensors
        bno085_poll();
        
        uint32_t now = to_ms_since_boot(get_absolute_time());

        // Process and Print Data every 1 second
        if (now - last_print > 1000) {
            // Read BMP280
            int32_t raw_temp, raw_press;
            bmp280_read_raw(&raw_temp, &raw_press);
            float temp = bmp280_convert_temp(raw_temp, &bmp_params) / 100.f;
            uint32_t press = bmp280_convert_pressure(raw_press, raw_temp, &bmp_params);

            // Print GPS
            printf("GPS: %s,%.5f,%.5f,%.1f\n",
                   GPS_GetTime(),
                   GPS_GetLat(),
                   GPS_GetLon(),
                   GPS_GetHeight());

            // Print BMP
            printf("BMP: Temp: %.2f C | Pres: %u Pa\n", temp, press);

            // Print BNO085
            if (bno085_get_report(&bno_state)) {
                printf("IMU: Acc:%.2f %.2f %.2f | Quat:%.2f %.2f %.2f %.2f\n",
                       bno_state.accel[0], bno_state.accel[1], bno_state.accel[2],
                       bno_state.quat[0], bno_state.quat[1], bno_state.quat[2], bno_state.quat[3]);
            }

            // Watchdog for BNO085 hang
            if (gpio_get(BNO085_INT_PIN)) {
                printf("BNO085 Hang Detected - Resetting...\n");
                sh2_devReset();
                i2c_bus_reset();
                enable_report(SH2_ROTATION_VECTOR, 10000);
            }

            last_print = now;
        }

        sleep_ms(10); 
    }
}