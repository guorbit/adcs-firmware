#ifndef GTU7_H
#define GTU7_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "hardware/uart.h"
#include "hardware/gpio.h"

#include "minmea.h"

#define GTU7_BAUD 9600
#define GTU7_RX 16 // gpio 16 rpi pico tx
#define GTU7_TX 17// gpio 17 rpi pico rx
#define GTU7_UART uart0

// struct to store gps data into
typedef struct {
    float lat;
    float lon;
    float speed;
    float alt;
    int hour;
    int min;
    int sec;
    int fix_quality;
} gps_data_t;

// function declarations
bool gps_init(uart_inst_t *uart, uint8_t tx, uint8_t rx, uint16_t baud);
gps_data_t gps_data(void);
bool read_uart(char *line, int max_length);
void gps_get_sentence(char *line); // Call this when data arrives


#endif