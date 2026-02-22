/*
 * Copyright © 2014 Kosma Moczek <kosma@cloudyourcar.com>
 * This program is free software. It comes without any warranty, to the extent
 * permitted by applicable law. You can redistribute it and/or modify it under
 * the terms of the Do What The Fuck You Want To Public License, Version 2, as
 * published by Sam Hocevar. See the COPYING file for more details.
 */

// my goat lol, based on https://github.com/kosma/minmea/blob/master/example.c
#include <stdint.h>

#include "minmea.h"
#include "gtu7.h"

// initialise storage struct
static gps_data_t gps_state = {0};

bool gps_init(uart_inst_t *uart, uint8_t tx, uint8_t rx, uint16_t baud) {
    uint16_t return_baud = uart_init(uart, baud);
    printf("baud rate: %d\n",return_baud);

	// set the TX and RX pins
	gpio_set_function(tx, UART_FUNCSEL_NUM(uart, tx));
	gpio_set_function(rx, UART_FUNCSEL_NUM(uart, rx));

	// set UART flow control CTS/RTS
	uart_set_hw_flow(uart, false, false);
    bool uart_status = uart_is_enabled(GTU7_UART);
    printf("uart enabled: %d\n", uart_status);
    return uart_status;
}

gps_data_t gps_data(void){
    return gps_state;
}

bool read_uart(char *line, int max_length){
    static uint16_t index = 0;

    while (uart_is_readable(GTU7_UART)){
        char getc = uart_getc(GTU7_UART);

        if (getc == '\n'){
            line[index] = '\0';
            index = 0;
            return true;
        } else if (getc != '\r' && index < max_length - 1){
            line[index++] = getc;
        }
    }
    return false;
}

void gps_get_sentence(char *line) {
    switch (minmea_sentence_id(line, false)) {
        case MINMEA_SENTENCE_RMC: {
            struct minmea_sentence_rmc frame;
            if (minmea_parse_rmc(&frame, line)) {
                // update the static struct, convert frame from struct to float
                gps_state.lat = minmea_tocoord(&frame.latitude);
                gps_state.lon = minmea_tocoord(&frame.longitude);
                gps_state.speed = minmea_tofloat(&frame.speed);
            }
        } break;
        
        case MINMEA_SENTENCE_GGA: {
            struct minmea_sentence_gga frame;
            if (minmea_parse_gga(&frame, line)) {
                gps_state.fix_quality = frame.fix_quality;
            }
        } break;
    }
}


