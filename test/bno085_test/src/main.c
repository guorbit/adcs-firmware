#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "bno085.h"
#include "sh2.h"

// i need to solder on a 1uf capacitor to prevent brownouts
void i2c_bus_reset() {
    printf("resetting i2c bus\n");
    // Disable I2C hardware briefly
    i2c_deinit(BNO085_I2C);
    
    // Manual toggle of SCL to clear stuck SDA
    gpio_init(BNO085_SCL_PIN);
    gpio_init(BNO085_SDA_PIN);
    gpio_set_dir(BNO085_SCL_PIN, GPIO_OUT);
    for (int i = 0; i < 10; i++) {
        gpio_put(BNO085_SCL_PIN, 0); sleep_us(5);
        gpio_put(BNO085_SCL_PIN, 1); sleep_us(5);
    }
    
    // Re-initialize I2C
    i2c_init(BNO085_I2C, 100 * 1000);
    gpio_set_function(BNO085_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(BNO085_SCL_PIN, GPIO_FUNC_I2C);
}

int main(void) {
    stdio_init_all();

    sleep_ms(3000);   // allow usb to enumerate
    printf("\nsystem boot\n");

    // i2c setup
    i2c_init(BNO085_I2C, 100 * 1000);
    gpio_set_function(BNO085_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(BNO085_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(BNO085_SDA_PIN);
    gpio_pull_up(BNO085_SCL_PIN);

    printf("Starting BNO085\n");

    // sensor initialisation   
    while(!bno085_init()) {
        printf("retry bno085 init\n");
        i2c_bus_reset;
        sh2_devReset;
        sleep_ms(1000);
    }

    // force reset the sensor for advert packet
    sh2_devReset();
    sleep_ms(200);

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
    if (!enable_report(SH2_MAGNETIC_FIELD_CALIBRATED, 10000)) {
        while(1){
            printf("could not enable magnetometer\n");
            sleep_ms(1000);
        }
    }

    printf("BNO085 ready v6\n");
    sleep_ms(100);

    uint32_t last_print = 0;
    uint32_t last_data_print = 0;
    bno085_state_t state;
    // main loop
    while (1) {
        bno085_poll();

        float qw, qx, qy, qz;
        if (bno085_get_report(&state)) {
            // ONLY print if 1000ms have passed
            uint32_t now = to_ms_since_boot(get_absolute_time());
            if (now - last_data_print > 1000) {

                printf("acc: %.2f %.2f %.2f | quat: %.2f %.2f %.2f %.2f | mag: %.1f %.1f %.1f\n",
                state.accel[0], state.accel[1], state.accel[2],
                state.quat[0],  state.quat[1],  state.quat[2], state.quat[3],
                state.mag[0],   state.mag[1],   state.mag[2]);

                last_data_print = now;
            }
        }
        
        if (reset_occurred) {
            printf("reset detected, re-enabling \n");
            sleep_ms(200);
            reset_occurred = false;
            
            i2c_bus_reset();

            bool reenabled = false;
            while(!reenabled){
                bno085_poll();
                if(enable_report(SH2_ROTATION_VECTOR, 10000)){
                    reenabled = true;
                } else {
                    printf(".");
                    sleep_ms(100);
                }
            }
        }

        sleep_ms(500);
        bno085_poll();
        if (to_ms_since_boot(get_absolute_time()) - last_print > 2000) {
            if (gpio_get(BNO085_INT_PIN)){
            printf("sensor Sleeping (int = 1). hard reset \n");
            sh2_devReset();
            reset_occurred = true; 
            }
            last_print = to_ms_since_boot(get_absolute_time());
        }

        sleep_ms(10);
    }
    
}
