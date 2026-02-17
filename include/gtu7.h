#ifndef GTU7_H
#define GTU7_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "hardware/uart.h"
#include "hardware/gpio.h"

#define GTU7_BAUD 9600

// struct to store gps data into
typedef struct {
    float lat;
    float lon;
    float speed;
    float alt;
    float time;
} gps_data_t;

// function declarations
void gps_init(uart_inst_t *uart, uint8_t tx, uint8_t rx, uint16_t baud);
gps_data_t gps_data(void);
void gps_get_sentence(char *line); // Call this when data arrives


#endif