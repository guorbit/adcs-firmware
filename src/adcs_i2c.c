#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"

#include "adcs_i2c.h"
#include "hardware/i2c.h"


void adcs_i2c_init(void) {
    i2c_init(ADCS_I2C, 100 * 1000);
    gpio_set_function(ADCS_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(ADCS_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(ADCS_SDA_PIN);
    gpio_pull_up(ADCS_SCL_PIN);
}
