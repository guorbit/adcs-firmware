#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 100KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments

// connection between OBC mcu and ADCS mcu
#define I2C_PORT_0 i2c0
#define I2C_SDA_0 8 // GPIO8
#define I2C_SCL_0 9 // GPIO9
// slave address of ADCS board ? arbitrary
#define SLAVE_ADDR 0x08


int main()
{
    stdio_init_all();

    // I2C Initialisation. Using it at 100Khz.
    i2c_init(I2C_PORT_0, 100*1000);
    // sets the function of the GPIO pins (from general to i2c)
    gpio_set_function(I2C_SDA_0, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_0, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_0);
    gpio_pull_up(I2C_SCL_0);
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c

    uint8_t msg[] = { 'h', 'i'};

    while (true) {
        // int i2c_write_blocking (i2c_inst_t *i2c, uint8_t addr, const uint8_t *src, size_t len, bool nostop)
        i2c_write_blocking(I2C_PORT_0, SLAVE_ADDR, msg, sizeof(msg), false)
        sleep_ms(1000);
    }
}
