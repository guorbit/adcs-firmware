#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/i2c_slave.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments

// this is for the sensors (pico is the master)
#define I2C_PORT_0 i2c0
#define I2C_SDA_0 8 // GPIO8
#define I2C_SCL_0 9 // GPIO9

// this is for connection with the OBC (pico is slave)
#define I2C_PORT_1 i2c1
#define I2C_SDA_1 14 // GPIO14
#define I2C_SCL_1 15 // GPIO15

// sensor addresses
#define BMP280_ADDR 0x76
#define ADCS_ADDR 0x08

uint8_t rx_data[32];

// slave handler
void i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event){
    static int idx = 0;

    switch (event){
        case I2C_SLAVE_RECEIVE:
            rx_data[idx++] = i2c_read_byte_raw(i2c);
            break;

        case I2C_SLAVE_FINISH:
            rx_data[idx] = '\0';
            printf("received: %s\n", rx_data);
            idx = 0;
            break;

        default:
            break;
    }
}

// write to register
void bmp280_write_reg(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    i2c_write_blocking(I2C_PORT_0, BMP280_ADDR, buf, 2, false);
}

// initialise BMP280
void bmp280_init_sensor() {
    bmp280_write_reg(0xF4, 0b00100111);
    bmp280_write_reg(0xF5, 0b00000000);
}

// read register
void bmp280_read_reg(uint8_t reg, uint8_t *buf, size_t len) {
    i2c_write_blocking(I2C_PORT_0, BMP280_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT_0, BMP280_ADDR, buf, len, false);
}

int main()
{
    stdio_init_all();

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT_0, 400*1000);
    i2c_init(I2C_PORT_1, 400*1000);
    // set pico as slave on i2c1 bus, 0x08 is an arbitrary address
    i2c_set_slave_mode(I2C_PORT_1, true, 0x08);

    // defines the pins i chose earlier as i2c pins
    gpio_set_function(I2C_SDA_0, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_0, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_0);
    gpio_pull_up(I2C_SCL_0);
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c

    i2c_slave_init(i2c1, ADCS_ADDR, &i2c_slave_handler);
    bmp280_init_sensor();

    while (true) {
        uint8_t data[6];
        bmp280_read_reg(0xF7, data, 6);

        int32_t adc_p = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
        int32_t adc_t = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);

        tight_loop_contents();

    }
}