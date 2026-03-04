#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "sh2.h"
#include "i2c_utils.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

#include "bno085.h"

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
    stdio_init_all();
    setup_uart();

    sleep_ms(10000);   // allow usb to enumerate
    printf("\nsystem boot\n");

    // reset the sensor by driving the reset pin low for 20ms, then releasing
    gpio_init(BNO085_RST_PIN);
    gpio_set_dir(BNO085_RST_PIN, GPIO_OUT);
    // drive low to reset
    gpio_put(BNO085_RST_PIN, 0);
    sleep_ms(20);
    // release
    gpio_put(BNO085_RST_PIN, 1);
    sleep_ms(2000);

    // i2c setup
    i2c_init(BNO085_I2C, 100 * 1000);
    gpio_set_function(BNO085_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(BNO085_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(BNO085_SDA_PIN);
    gpio_pull_up(BNO085_SCL_PIN);

    // print devices visible on the bus
    i2c_bus_scan(BNO085_I2C, BNO085_SDA_PIN, BNO085_SCL_PIN);

    printf("Starting BNO085\n");

    // sensor initialisation   
    while(!bno085_init()) {
        printf("retry bno085 init\n");
        i2c_bus_reset(BNO085_I2C, BNO085_SDA_PIN, BNO085_SCL_PIN);
        sh2_devReset();
        sleep_ms(1000);
    }

    // force reset the sensor for advert packet
    // sh2_devReset();
    // sleep_ms(200);

    for (int i = 0; i < 200; i++) {
        // printf("int pin state: %d \n", gpio_get(BNO085_INT_PIN));
        bno085_poll();  // poll sensor for advert packet
        sleep_ms(100);
    }

    printf("sensor ready.\n");

    // enable different reports, at 100 hz
    if (!enable_report(SH2_ROTATION_VECTOR, 10000)) {
        while(1){
            printf("could not enable rotation vector\n");
            sleep_ms(1000);
        }
    }
    if (!enable_report(SH2_ACCELEROMETER, 10000)) {
        while(1){
            printf("could not enable accelerometer\n");
            sleep_ms(1000);
        }
    }
    if (!enable_report(SH2_MAGNETIC_FIELD_CALIBRATED, 50000)) {
        while(1){
            printf("could not enable magnetometer\n");
            sleep_ms(1000);
        }
    }

    printf("BNO085 ready v6\n");
    sleep_ms(100);

    bno085_state_t state;
    uint32_t last_sensor_read = to_ms_since_boot(get_absolute_time()); // For the watchdog
    uint32_t last_data_print = 0; // For printing
    static float last_qx, last_qy, last_qz;
    static int stale_count = 0;
    // main loop
    while (1) {
        bno085_poll();

        // timer for print and watchdog
        uint32_t now = to_ms_since_boot(get_absolute_time());

        float qw, qx, qy, qz;
        if (bno085_get_report(&state)) {
            // reset watchdog
            last_sensor_read = now;

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

                printf("status: %d | acc: %.2f %.2f %.2f | quat: %.2f %.2f %.2f %.2f | mag: %.1f %.1f %.1f\n",
                    state.status[0],
                    state.accel[0], state.accel[1], state.accel[2],
                    state.quat[0],  state.quat[1],  state.quat[2], state.quat[3],
                    state.mag[0],   state.mag[1],   state.mag[2]);

                last_data_print = now;
            }
        }

        // reset stuffs
        if (now - last_sensor_read > 5000) {
            printf("watchdog timeout, resetting hardware\n");
            if (bno085_hw_reset()) {
                // reset timer so we don't meet if statement condition immediately and get stuck
                last_sensor_read = now;
            } else {
                printf("bno085_hw_reset failed, starting next loop\n");
                last_sensor_read = now;
                continue;
            }
        } else if (reset_occurred) {
            bno085_reset(); // resets reports and the flag
            last_sensor_read = now;
        } 
        
        sleep_ms(5);
    }
    
}
