#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "sh2.h"
#include "i2c_utils.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"


#include "bmp280.h"
#include "bno085.h"
#include "obc.h"

#define GTU7_BAUD 9600
#define GTU7_RX   16 // Pico TX (Connect to GPS RX)
#define GTU7_TX   17 // Pico RX (Connect to GPS TX)
#define GTU7_UART uart0

void setup_uart() {
    // Initialize UART at the specified baud rate
    uart_init(GTU7_UART, GTU7_BAUD);

    // Set the GPIO pin mux to the UART function
    gpio_set_function(GTU7_TX, GPIO_FUNC_UART);
    gpio_set_function(GTU7_RX, GPIO_FUNC_UART);
}

int main(void) {
    // Initialize UART for debugging output
    stdio_init_all();
    setup_uart(); // for gps
    sleep_ms(10000);   // allow usb to enumerate
    printf("\nsystem boot\n");

    adcs_slave_init();
    printf("adcs initialised as slave\n");

    // reset the sensor by driving the reset pin low for 20ms, then releasing
    gpio_init(BNO085_RST_PIN);
    gpio_set_dir(BNO085_RST_PIN, GPIO_OUT);
    // drive low to reset
    gpio_put(BNO085_RST_PIN, 0);
    sleep_ms(20);
    // release
    gpio_put(BNO085_RST_PIN, 1);
    sleep_ms(2000);

    // init i2c for bmp280 and bno085
    i2c_init(BNO085_I2C, 100 * 1000);
    gpio_set_function(BNO085_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(BNO085_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(BNO085_SDA_PIN);
    gpio_pull_up(BNO085_SCL_PIN);
    
    printf("I2C initialized on pins SDA=%d, SCL=%d\n", PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN);
    // print devices visible on the bus
    i2c_bus_scan(BNO085_I2C, BNO085_SDA_PIN, BNO085_SCL_PIN);

    // bno085 initialisation
    printf("Starting BNO085\n");
    while(!bno085_init()) {
        printf("retry bno085 init\n");
        i2c_bus_reset(BNO085_I2C, BNO085_SDA_PIN, BNO085_SCL_PIN);
        sh2_devReset();
        sleep_ms(1000);
    }

    for (int i = 0; i < 200; i++) {
        // printf("int pin state: %d \n", gpio_get(BNO085_INT_PIN));
        bno085_poll();  // poll sensor for advert packet
        sleep_ms(100);
    }

    // enable different reports, at 100 hz
    while(!enable_report(SH2_ROTATION_VECTOR, 10000)){
        printf("could not enable rotation vector\n");
        sleep_ms(50);
    }
    while(!enable_report(SH2_ACCELEROMETER, 10000)){
        printf("could not enable accelerometer\n");
        sleep_ms(50);
    }
    while(!enable_report(SH2_MAGNETIC_FIELD_CALIBRATED, 50000)){
        printf("could not enable magnetometer\n");
        sleep_ms(1000);
    }

    printf("BNO085 ready v6\n");
    sleep_ms(100);
    
    // bmp280 initialisation
    // test i2c communication - try to read from bmp280
    uint8_t test_buf[1];
    int ret = i2c_read_blocking(i2c_default, 0x76, test_buf, 1, false);
    printf("bmp280 I2C read test (addr 0x76): %s\n", ret >= 0 ? "SUCCESS" : "FAILED");
    
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
    
    // init stuff that will be used in while loop
    bno085_state_t state;
    uint32_t last_sensor_read = to_ms_since_boot(get_absolute_time()); // for the watchdog
    uint32_t last_data_print = 0; // for printing
    static float last_qx, last_qy, last_qz;
    static int stale_count = 0;
    int32_t raw_temp, raw_pressure;
    char obc_telem [256]; // internal buffer

    // main loop
    while (1) {
        bmp280_read_raw(&raw_temp, &raw_pressure);
        
        int32_t temp_c = bmp280_convert_temp(raw_temp, &calib_params);
        uint32_t pressure_pa = bmp280_convert_pressure(raw_pressure, raw_temp, &calib_params);
        float temperature = temp_c / 100.0f;
        
        // printf("temp: %.3f pressure: %lu\n", temperature, pressure_pa);
        
        bno085_poll();
        // timer for print and watchdog
        uint32_t now = to_ms_since_boot(get_absolute_time());
        float qw, qx, qy, qz;

        if (bno085_get_report(&state)) {
            // reset watchdog
            last_sensor_read = now;
            
            // soft reset, for stale data
            if (state.status == 0){
                if (++stale_count > 100) {
                    printf("data unreliable for 1s, running sh2_devReset\n");
                    sh2_devReset();
                    // re-init sensor ?
                    stale_count = 0; // reset flag
                }
            } else {
                // reset flag, otherwise it's pretty easy to get stale_count == 0 and trigger a reset
                stale_count = 0; 
            }
            
            // print data, rate limited atm
            if (now - last_data_print > 1000) {

                // temp: %07.2f | pressure: %lu | bno085 status: %d | acc: %+07.2f %+07.2f %+07.2f | quat: %+07.2f %+07.2f %+07.2f %+07.2f | mag: %+07.2f %+07.2f %+07.2f\n"
                // snprintf uses the 
                uint16_t obc_msg_len = snprintf(obc_telem, sizeof(obc_telem), "t%07.2f|b%lu|i%d|a%+07.2f %+07.2f %+07.2f|q%+07.2f %+07.2f %+07.2f %+07.2f|m%+07.2f %+07.2f %+07.2f\n",
                    temperature, pressure_pa, state.status[0],
                    state.accel[0], state.accel[1], state.accel[2],
                    state.quat[0],  state.quat[1],  state.quat[2], state.quat[3],
                    state.mag[0],   state.mag[1],   state.mag[2]);

                last_data_print = now;

                printf("length of buffer: %d\n", obc_msg_len); // currently 103

                adcs_telemetry((const uint8_t *)obc_telem, obc_msg_len);
            }
        }

        // reset stuffs
        if (now - last_sensor_read > 50000) {
            printf("watchdog timeout, resetting hardware\n");
            if (bno085_hw_reset()) {
                // reset timer so we don't meet if statement condition immediately and get stuck
                last_sensor_read = to_ms_since_boot(get_absolute_time());
            } else {
                printf("bno085_hw_reset failed, starting next loop\n");
                last_sensor_read = to_ms_since_boot(get_absolute_time());
                continue;
            }
        } else if (reset_occurred) {
            bno085_reset(); // resets reports and the flag
            last_sensor_read = now;
        } 
        
        sleep_ms(5);
    }
    
}