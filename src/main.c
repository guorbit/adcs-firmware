// #include <stdio.h>
// #include <stdint.h>
// #include "pico/stdlib.h"
// #include "hardware/i2c.h"
// #include "hardware/uart.h"
// #include "pico/binary_info.h"
// #include "sh2.h"

// // sensor headers
// #include "bmp280.h"
// #include "bno085.h"
// #include "sh2.h"
// #include "GPS.h"

// // because the bno085 keeps browning out, i will add some capacitors
// void i2c_bus_reset() {
//     printf("resetting i2c bus\n");
//     i2c_deinit(BNO085_I2C);

//     gpio_init(BNO085_SCL_PIN);
//     gpio_init(BNO085_SDA_PIN);
//     gpio_set_dir(BNO085_SCL_PIN, GPIO_OUT);

//     for (int i = 0; i < 10; i++) {
//         gpio_put(BNO085_SCL_PIN, 0);
//         sleep_ms(5);
//         gpio_put(BNO085_SCL_PIN, 1);
//         sleep_ms(5);
//     }

//     i2c_init(BNO085_I2C, 100 * 1000);
//     gpio_set_function(BNO085_SDA_PIN, GPIO_FUNC_I2C);
//     gpio_set_function(BNO085_SCL_PIN, GPIO_FUNC_I2C);
// }

// // void?
// int main(void) {
//     stdio_init_all();
//     sleep_ms(3000); // Wait for USB serial
//     printf("\nsystem boot\n");

//     // initialize i2c (BMP280 and BNO085 share a bus)
//     i2c_init(BNO085_I2C, 100 * 1000);
//     gpio_set_function(BNO085_SDA_PIN, GPIO_FUNC_I2C);
//     gpio_set_function(BNO085_SCL_PIN, GPIO_FUNC_I2C);
//     gpio_pull_up(BNO085_SDA_PIN);
//     gpio_pull_up(BNO085_SCL_PIN);

//     // // initialize gps (uart)
//     // GPS_Init(uart0, 0, 1, 9600);

//     // // initialize BMP280
//     // bmp280_init();
//     // // defined in bmp280.h
//     // struct bmp280_calib_param bmp_params;
//     // bmp280_get_calib_params(&bmp_params);
//     // printf("BMP280 Initialized.\n");

//     // initialize BNO085
//     while (!bno085_init()) {
//         printf("retrying bno085 init\n");
//         i2c_bus_reset();
//         sh2_devReset();
//         sleep_ms(1000);
//     }

//     sh2_devReset();
//     sleep_ms(200);

//     for (int i = 0; i < 200; i++) {
//         bno085_poll(); // poll sensor for advert packet
//         sleep_ms(100);
//     }

//     printf("advert packet read");

//     // enable different reports, at 100 hz
//     if (!enable_report(SH2_ROTATION_VECTOR, 10000)) {
//         while (1) {
//             printf("could not enable rotation vector\n");
//             sleep_ms(1000);
//         }
//     }
//     if (!enable_report(SH2_ACCELEROMETER, 10000)) {
//         while (1) {
//             printf("could not enable accelerometer\n");
//             sleep_ms(1000);
//         }
//     }
//     if (!enable_report(SH2_MAGNETIC_FIELD_CALIBRATED, 10000)) {
//         while (1) {
//             printf("could not enable magnetometer\n");
//             sleep_ms(1000);
//         }
//     }

//     printf("reports enabled\n");
//     sleep_ms(100);

//     uint32_t last_print = 0;
//     bno085_state_t bno_state;

//     while (1)
//     {
//         // poll sensor
//         bno085_poll();

//         uint32_t now = to_ms_since_boot(get_absolute_time());

//         // print data every 1 second
//         if (now - last_print > 1000) {
//             // // read BMP280
//             // int32_t raw_temp, raw_press;
//             // bmp280_read_raw(&raw_temp, &raw_press);
//             // float temp = bmp280_convert_temp(raw_temp, &bmp_params) / 100.f;
//             // uint32_t press = bmp280_convert_pressure(raw_press, raw_temp, &bmp_params);

//             // // print GPS
//             // printf("GPS: %s,%.5f,%.5f,%.1f\n",
//             //        GPS_GetTime(),
//             //        GPS_GetLat(),
//             //        GPS_GetLon(),
//             //        GPS_GetHeight());

//             // // print BMP
//             // printf("BMP: Temp: %.2f C | Pres: %u Pa\n", temp, press);

//             // print BNO085
//             if (bno085_get_report(&bno_state))
//             {
//                 printf("acc: %.2f %.2f %.2f | quat: %.2f %.2f %.2f %.2f | mag: %.1f %.1f %.1f\n",
//                        bno_state.accel[0], bno_state.accel[1], bno_state.accel[2],
//                        bno_state.quat[0], bno_state.quat[1], bno_state.quat[2], bno_state.quat[3],
//                        bno_state.mag[0], bno_state.mag[1], bno_state.mag[2]);
//             }
//             last_print = now;
//         }

//         sleep_ms(500);
//         bno085_poll();
//         if (to_ms_since_boot(get_absolute_time()) - last_print > 2000)
//         {
//             if (gpio_get(BNO085_INT_PIN) == 1)
//             {
//                 printf("sensor Sleeping (int = 1). hard reset \n");
//                 sh2_devReset();
//                 reset_occurred = true;
//             }
//             last_print = to_ms_since_boot(get_absolute_time());
//         }

//         sleep_ms(10);

//         // if bno085 starts crashing
//         if (reset_occurred)
//         {
//             printf("reset detected, re-enabling \n");
//             sleep_ms(200);
//             reset_occurred = false;

//             i2c_bus_reset();

//             bool reenabled = false;
//             while (!reenabled)
//             {
//                 bno085_poll();
//                 if (!enable_report(SH2_ROTATION_VECTOR, 10000))
//                 {
//                     printf("could not enable rotation vector\n");
//                 }
//                 else if (!enable_report(SH2_ACCELEROMETER, 10000))
//                 {
//                     printf("could not enable accelerometer\n");
//                 }
//                 else if (!enable_report(SH2_MAGNETIC_FIELD_CALIBRATED, 10000))
//                 {
//                     printf("could not enable magnetometer\n");
//                 }
//                 else
//                 {
//                     reenabled = true;
//                     printf("re-enabled\n");
//                 }
//             }
//             // update watchdog timer so we don't hard reset immediately
//             last_print = to_ms_since_boot(get_absolute_time());
//         }

//         sleep_ms(500);
//         bno085_poll(); // ask sh2 to process any pending transfers

//         // watchdog, triggers every 2000ms
//         if (to_ms_since_boot(get_absolute_time()) - last_print > 2000)
//         {
//             if (gpio_get(BNO085_INT_PIN) == 1) {
//                 printf("sensor Sleeping (int = 1). hard reset \n");
//                 sh2_devReset();
//                 reset_occurred = true;
//             }
//             last_print = to_ms_since_boot(get_absolute_time());
//         }

//         sleep_ms(10);
//     }
// }

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "bmp280.h"
#include "bno085.h"
#include "sh2.h"

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
        
        sleep_ms(1000);
    }
    
    return 0;
    
}
