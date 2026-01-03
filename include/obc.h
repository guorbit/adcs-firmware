#ifndef OBC_H
#define OBC_H

#include <stddef.h>
#include <stdint.h>

// connection between OBC mcu and ADCS mcu (ADCS is slave)
#define ADCS_PORT i2c1
#define ADCS_SDA 14 // GPIO14
#define ADCS_SCL 15 // GPIO15
#define ADCS_ADDR 0x08

#define TX_BUF_SIZE 32 // matching obc i think ??? need to check their header

void adcs_slave_init(void);
void adcs_telemetry(const uint8_t *data, size_t len);

#endif