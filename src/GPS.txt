#include "GPS.h"
#include <minmea.h>
#include <cstdio>
#include <cstring>

// Static singleton pointer?
GPS * GPS::pSelf = NULL;

GPS::GPS() {
	//constructor stub initialy empty

}

GPS::~GPS() {
	//destructor stub initially empty
}

// singleton accssor latest?
GPS * GPS::getInstance(){
	if (GPS::pSelf == NULL){
		GPS::pSelf = new GPS;
	}
	return GPS::pSelf ;
}

//Setup the UART for GPS use
void GPS::setUp(uart_inst_t *uart, uint8_t tx, uint8_t rx, uint16_t baud){
	pUart = uart;

	 uart_init(pUart, baud);

	// Set the TX and RX pins
	gpio_set_function(tx, UART_FUNCSEL_NUM(pUart, tx));
	gpio_set_function(rx, UART_FUNCSEL_NUM(pUart, rx));

	// Set UART flow control CTS/RTS
	uart_set_hw_flow(pUart, false, false);

	// Determine the UART IRQ number
	int UART_IRQ = pUart == uart0 ? UART0_IRQ : UART1_IRQ;

	// reister and enable UART RX interrupt
	irq_set_exclusive_handler(UART_IRQ, GPS::onUartRxCB);
	irq_set_enabled(UART_IRQ, true);

	// RX interupt only
	uart_set_irq_enables(pUart, true, false);

}
//static callback wrapper
void GPS::onUartRxCB(){
	GPS::getInstance()->onUartRx();
}
/*UART RX interrupt handler
  Reads incoming characters into a buffer untill new  line or carroiage 
  Then deoces it
*/
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

//Decode the NMEA sentence in the buffer
void GPS::decodeBuffer(){
	switch (minmea_sentence_id(xBuffer, false)) {
		//Position, Velocity, and Time
		case MINMEA_SENTENCE_RMC: {
			struct minmea_sentence_rmc frame;

			if (minmea_parse_rmc(&frame, xBuffer)) {
				xLat = minmea_tocoord(&frame.latitude);
				xLon = minmea_tocoord(&frame.longitude);
				xSpeed = minmea_tofloat(&frame.speed);
				
				snprintf(xTime, sizeof(xTime), "%02d:%02d:%02d", 
					frame.time.hours, frame.time.minutes, frame.time.seconds);

				}
			} break;
			
		//Satellites in view
			case MINMEA_SENTENCE_GSV: {
				struct minmea_sentence_gsv frame;

				if (minmea_parse_gsv(&frame, xBuffer)) {
					xSat = frame.total_sats;
				
				}
			} break;

		//Global Positioning System Fix Data	
		case MINMEA_SENTENCE_GGA: {
			struct minmea_sentence_gga frame;

			if (minmea_parse_gga(&frame, xBuffer)) {
				xHeight = minmea_tofloat(&frame.altitude);
			}
		} break;

		}
}

//getter functions

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

