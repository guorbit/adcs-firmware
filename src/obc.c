#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "obc.h"
// #include "hardware/i2c_slave.h" <- wrong version or something
#include "pico/i2c_slave.h"

// needs to allocate permanent memory, volatile so compiler doesn't assume things
static volatile uint8_t tx_buf[TX_BUF_SIZE];
static volatile uint8_t tx_len = 0;
static volatile uint8_t tx_idx = 0;
static volatile bool tx_done = false;

// slave handler for pico, as a slave to the OBC
static void adcs_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event) {
    switch (event) {
        case I2C_SLAVE_REQUEST: // when obc requests data from adcs, from pico sdk
            if (tx_idx >= tx_len) {
                tx_idx = 0;
            }
            i2c_write_raw_blocking(i2c, (const uint8_t *)&tx_buf[tx_idx], 1); // write raw for slave
            tx_idx++;
            break;

        case I2C_SLAVE_FINISH:
            tx_idx = 0;
            break;

        default:
            break;
    }
}

void adcs_telemetry(const uint8_t *data, size_t len){
    // so stuffs isn't too long
    if (len > TX_BUF_SIZE) len = TX_BUF_SIZE;
    memset((void *)tx_buf, 0, TX_BUF_SIZE);
    memcpy((void *)tx_buf, data, len);
    tx_len = len;
}

// initialisation
void adcs_slave_init(void)
{
    stdio_init_all();

    // I2C Initialisation. Using it at 50Khz for better reliability.
    i2c_init(ADCS_PORT, 100*1000);
    // sets the function of the GPIO pins (from general to i2c)
    gpio_set_function(ADCS_SDA, GPIO_FUNC_I2C);
    gpio_set_function(ADCS_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(ADCS_SDA);
    gpio_pull_up(ADCS_SCL);

    i2c_slave_init(ADCS_PORT, ADCS_ADDR, adcs_slave_handler);

    const char *msg = "ADCS test data 1234567890ABCDEFG"; // 32 bytes
    adcs_telemetry((const uint8_t *)msg, strlen(msg));
}

