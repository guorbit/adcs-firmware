#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "i2c_utils.h"

// Needed for i2c_bus_scan
// I2C reserves some addresses for special purposes. We exclude these from the scan.
// These are any addresses of the form 000 0xxx or 111 1xxx
bool reserved_addr(uint8_t addr) {
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

// I2C bus scan: Only works if the bus is initialised with 
//  the correct pins and pull-ups, so call this after 
//  i2c_init and gpio setup
int i2c_bus_scan(i2c_inst_t *i2c, int sda_pin, int scl_pin) {
    // Make the I2C pins available to picotool
    //bi_decl(bi_2pins_with_func(8, 9, GPIO_FUNC_I2C));

    printf("\nI2C Bus Scan\n");
    printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
    
    for (int addr = 0; addr < (1 << 7); ++addr) {
        if (addr % 16 == 0) {
            printf("%02x ", addr);
        }

        // Perform a 1-byte dummy read from the probe address. If a slave
        // acknowledges this address, the function returns the number of bytes
        // transferred. If the address byte is ignored, the function returns
        // -1.

        // Skip over any reserved addresses.
        int ret;
        uint8_t rxdata;
        if (reserved_addr(addr))
            ret = PICO_ERROR_GENERIC;
        else
            ret = i2c_read_blocking(i2c, addr, &rxdata, 1, false);

        printf(ret < 0 ? "." : "@");
        printf(addr % 16 == 15 ? "\n" : "  ");
    }
    printf("Done.\n");
    return 0;
}

void i2c_bus_reset(i2c_inst_t *i2c, int sda_pin, int scl_pin) {
    printf("resetting i2c bus\n");
    // Disable I2C hardware briefly
    i2c_deinit(i2c);
    
    // Manual toggle of SCL to clear stuck SDA
    gpio_init(scl_pin);
    gpio_init(sda_pin);
    gpio_set_dir(scl_pin, GPIO_OUT);
    for (int i = 0; i < 10; i++) {
        gpio_put(scl_pin, 0); sleep_us(5);
        gpio_put(scl_pin, 1); sleep_us(5);
    }
    
    // Re-initialize I2C
    i2c_init(i2c, 100 * 1000);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
}
