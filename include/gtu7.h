#ifndef GTU7_H
#define GTU7_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "hardware/uart.h"
#include "hardware/gpio.h"

#define GTU7_BAUD 9600
#define GTU7_RX 16 // gpio 16 rpi pico tx
#define GTU7_TX 17// gpio 17 rpi pico rx
#define GTU7_UART uart0


#ifndef MINMEA_MAX_SENTENCE_LENGTH
#define MINMEA_MAX_SENTENCE_LENGTH 80

// struct to store gps data into
typedef struct {
    float lat;
    float lon;
    float speed;
    float alt;
    float time;
} gps_data_t;

// function declarations
bool gps_init(uart_inst_t *uart, uint8_t tx, uint8_t rx, uint16_t baud);
gps_data_t gps_data(void);
void gps_get_sentence(char *line); // Call this when data arrives


#endif