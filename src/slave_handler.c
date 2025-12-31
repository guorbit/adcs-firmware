#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/i2c_slave.h"

#define RX_BUF_SIZE 32

// slave handler for pico, as a slave to the OBC

// needs to allocate permanent memory, volatile so compiler doesn't assume things
static volatile uint8_t rx_data[RX_BUF_SIZE];
static volatile size_t rx_len = 0;
static volatile bool rx_done = false;

void i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event) {
    switch (event) {
        case I2C_SLAVE_RECEIVE:
            if (rx_len < RX_BUF_SIZE - 1) {
                rx_data[rx_len++] = i2c_read_byte_raw(i2c);
            }
            break;

        case I2C_SLAVE_FINISH:
            rx_data[rx_len] = '\0';
            rx_done = true;
            rx_len = 0;
            break;

        default:
            break;
    }
}
