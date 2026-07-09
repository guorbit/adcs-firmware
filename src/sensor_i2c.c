#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"

#include "sensor_i2c.h"
#include "hardware/i2c.h"


void sensor_i2c_init(void) {
    gpio_init(SENSOR_SDA);
    gpio_init(SENSOR_SCL);
    gpio_set_function(SENSOR_SDA, GPIO_FUNC_I2C);
    gpio_set_function(SENSOR_SCL, GPIO_FUNC_I2C);
    i2c_init(SENSOR_I2C, 100 * 1000);
    // gpio_pull_up(SENSOR_SDA);
    // gpio_pull_up(SENSOR_SCL);
}
