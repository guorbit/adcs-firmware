#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/i2c_slave.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments

// this is for connection with the OBC (pico is slave)
#define I2C_PORT_1 i2c1
#define I2C_SDA_1 14 // GPIO14
#define I2C_SCL_1 15 // GPIO15
#define ADCS_ADDR 0x08


char mem[32] = "hello, hello again, hello world"; // buffer
uint8_t mem_address;
// char test_data[32] = "0123456789ABCDEFGHIJKL";
bool mem_address_written;


// slave handler
void i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event){

    switch (event){
        case I2C_SLAVE_REQUEST:
            i2c_write_byte_raw(i2c1, mem[mem_address]);
            mem_address++;
            break;

        case I2C_SLAVE_FINISH:
            mem_address_written = false;
            break;

        default:
            break;
    }
}

static void setup_slave() {
    // I2C Initialisation. Using it at 400Khz.
    
    // set pico as slave on i2c1 bus, 0x08 is an arbitrary address
    gpio_init(I2C_SDA_1);
    i2c_slave_init(I2C_PORT_1, ADCS_ADDR, &i2c_slave_handler);
    gpio_set_function(I2C_SDA_1, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_1);

    gpio_init(I2C_SCL_1);
    gpio_set_function(I2C_SCL_1, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SCL_1);

    i2c_init(I2C_PORT_1, 400*1000);
    i2c_slave_init(i2c1, ADCS_ADDR, &i2c_slave_handler);
}

int main()
{
    stdio_init_all();
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c

    while (true) {
        sleep_ms(50);
    }

}
