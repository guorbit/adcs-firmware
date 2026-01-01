/*
 * GPS.cpp
 *
 *  Created on: 3 Jan 2025
 *      Author: jondurrant
 */

#include "GPS.h"

#include <minmea.h>
#include <cstdio>
#include <cstring>

GPS * GPS::pSelf = NULL;

GPS::GPS() {
	// TODO Auto-generated constructor stub

}

GPS::~GPS() {
	// TODO Auto-generated destructor stub
}


GPS * GPS::getInstance(){
	if (GPS::pSelf == NULL){
		GPS::pSelf = new GPS;
	}
	return GPS::pSelf ;
}

void GPS::setUp(uart_inst_t *uart, uint8_t tx, uint8_t rx, uint16_t baud){
	pUart = uart;

	 uart_init(pUart, baud);

	// Set the TX and RX pins
	gpio_set_function(tx, UART_FUNCSEL_NUM(pUart, tx));
	gpio_set_function(rx, UART_FUNCSEL_NUM(pUart, rx));

	//int __unused actual = uart_set_baudrate(pUart, baud);

	// Set UART flow control CTS/RTS
	uart_set_hw_flow(pUart, false, false);

	// Turn off FIFO's
	//uart_set_fifo_enabled(pUart, false);

	// Set up a RX interrupt
	// We need to set up the handler first
	// Select correct interrupt for the UART we are using
	int UART_IRQ = pUart == uart0 ? UART0_IRQ : UART1_IRQ;

	// And set up and enable the interrupt handlers
	irq_set_exclusive_handler(UART_IRQ, GPS::onUartRxCB);
	irq_set_enabled(UART_IRQ, true);

	// Now enable the UART to send interrupts - RX only
	uart_set_irq_enables(pUart, true, false);

}

void GPS::onUartRxCB(){
	GPS::getInstance()->onUartRx();
}

void GPS::onUartRx(){
	while (uart_is_readable(pUart)) {
		uint8_t ch = uart_getc(pUart);
		if ((ch == '\n') ||  ( ch == '\r')){
			if (xBufferInd > 0){
				xBuffer[xBufferInd++] = 0;
				decodeBuffer();
				xBufferInd = 0;
			}
		} else {
			xBuffer[xBufferInd++] = ch;
		}
	}
}


void GPS::decodeBuffer(){
	switch (minmea_sentence_id(xBuffer, false)) {
		case MINMEA_SENTENCE_RMC: {
			struct minmea_sentence_rmc frame;
			if (minmea_parse_rmc(&frame, xBuffer)) {
				xLat = minmea_tocoord(&frame.latitude);
				xLon = minmea_tocoord(&frame.longitude);
				xSpeed = minmea_tofloat(&frame.speed);
				snprintf(xTime, sizeof(xTime), "%02d:%02d:%02d", 
					frame.time.hours, frame.time.minutes, frame.time.seconds);

				/*
				printf( "$xxRMC: raw coordinates and speed: (%d/%d,%d/%d) %d/%d\n",
							frame.latitude.value, frame.latitude.scale,
							frame.longitude.value, frame.longitude.scale,
							frame.speed.value, frame.speed.scale);
					printf( "$xxRMC fixed-point coordinates and speed scaled to three decimal places: (%d,%d) %d\n",
							minmea_rescale(&frame.latitude, 1000),
							minmea_rescale(&frame.longitude, 1000),
							minmea_rescale(&frame.speed, 1000));
					printf( "$xxRMC floating point degree coordinates and speed: (%f,%f) %f\n",
							minmea_tocoord(&frame.latitude),
							minmea_tocoord(&frame.longitude),
							minmea_tofloat(&frame.speed));
				*/
				}
				else {
				//	printf( "$xxRMC sentence is not parsed\n");
				}
			} break;

			/*
			} break;
			 */

			/*
			case MINMEA_SENTENCE_GST: {
				struct minmea_sentence_gst frame;
				if (minmea_parse_gst(&frame, xBuffer)) {
					printf( "$xxGST: raw latitude,longitude and altitude error deviation: (%d/%d,%d/%d,%d/%d)\n",
							frame.latitude_error_deviation.value, frame.latitude_error_deviation.scale,
							frame.longitude_error_deviation.value, frame.longitude_error_deviation.scale,
							frame.altitude_error_deviation.value, frame.altitude_error_deviation.scale);
					printf( "$xxGST fixed point latitude,longitude and altitude error deviation"
						   " scaled to one decimal place: (%d,%d,%d)\n",
							minmea_rescale(&frame.latitude_error_deviation, 10),
							minmea_rescale(&frame.longitude_error_deviation, 10),
							minmea_rescale(&frame.altitude_error_deviation, 10));
					printf( "$xxGST floating point degree latitude, longitude and altitude error deviation: (%f,%f,%f)",
							minmea_tofloat(&frame.latitude_error_deviation),
							minmea_tofloat(&frame.longitude_error_deviation),
							minmea_tofloat(&frame.altitude_error_deviation));
				}
				else {
					printf( "$xxGST sentence is not parsed\n");
				}
			} break;
			*/

			case MINMEA_SENTENCE_GSV: {
				struct minmea_sentence_gsv frame;
				if (minmea_parse_gsv(&frame, xBuffer)) {
					xSat = frame.total_sats;
					/*
					printf( "$xxGSV: message %d of %d\n", frame.msg_nr, frame.total_msgs);
					printf( "$xxGSV: satellites in view: %d\n", frame.total_sats);
					for (int i = 0; i < 4; i++)
						printf( "$xxGSV: sat nr %d, elevation: %d, azimuth: %d, snr: %d dbm\n",
							frame.sats[i].nr,
							frame.sats[i].elevation,
							frame.sats[i].azimuth,
							frame.sats[i].snr);
							*/
				}
				else {
				//	printf( "$xxGSV sentence is not parsed\n");
				}
			} break;

		case MINMEA_SENTENCE_GGA: {
			struct minmea_sentence_gga frame;
			if (minmea_parse_gga(&frame, xBuffer)) {
				xHeight = minmea_tofloat(&frame.altitude);
			}
		} break;

			/*
			case MINMEA_SENTENCE_VTG: {
			   struct minmea_sentence_vtg frame;
			   if (minmea_parse_vtg(&frame, xBuffer)) {
					printf( "$xxVTG: true track degrees = %f\n",
						   minmea_tofloat(&frame.true_track_degrees));
					printf( "        magnetic track degrees = %f\n",
						   minmea_tofloat(&frame.magnetic_track_degrees));
					printf( "        speed knots = %f\n",
							minmea_tofloat(&frame.speed_knots));
					printf( "        speed kph = %f\n",
							minmea_tofloat(&frame.speed_kph));
			   }
			   else {
					printf( "$xxVTG sentence is not parsed\n");
			   }
			} break;
			*/

			/*

			case MINMEA_SENTENCE_ZDA: {
				struct minmea_sentence_zda frame;
				if (minmea_parse_zda(&frame, xBuffer)) {
					printf( "$xxZDA: %d:%d:%d %02d.%02d.%d UTC%+03d:%02d\n",
						   frame.time.hours,
						   frame.time.minutes,
						   frame.time.seconds,
						   frame.date.day,
						   frame.date.month,
						   frame.date.year,
						   frame.hour_offset,
						   frame.minute_offset);
				}
				else {
				//	printf( "$xxZDA sentence is not parsed\n");
				}
			} break;
			*/

			/*
			case MINMEA_INVALID: {
				printf( "$xxxxx sentence is not valid\n");
			} break;

			default: {
				printf( "$xxxxx sentence is not parsed\n");
			} break;
			*/
		}
}


float GPS::getLat(){
	return xLat;
}
float GPS::getLon(){
	return xLon;
}
float GPS::getSpeed(){
	return xSpeed;
}
int GPS::getNumSat(){
	return xSat;
}

const char* GPS::getTime(){
	return xTime;
}

float GPS::getHeight(){
	return xHeight;
}

