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

void gps_init(uart_inst_t *uart, uint8_t tx, uint8_t rx, uint16_t baud) {
	uart_init(uart, baud);

	// set the TX and RX pins
	gpio_set_function(tx, UART_FUNCSEL_NUM(uart, tx));
	gpio_set_function(rx, UART_FUNCSEL_NUM(uart, rx));

	// set UART flow control CTS/RTS
	uart_set_hw_flow(uart, false, false);
}

gps_data_t gps_data(void){
    return gps_state;
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
                frame.fix_quality = frame.fix_quality;
            }
        } break;
    }
}


