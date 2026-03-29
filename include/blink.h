#ifndef BLINK_H
#define BLINK_H

#include "pico/stdlib.h"

#define LED_PIN PICO_DEFAULT_LED_PIN
#define LED_BASE_INTERVAL 100 // basic time interval in ms
#define LED_INTERVAL_ON 1 // no. of intervals the LED is on
#define LED_INTERVAL_TOTAL 5 // total amount of base intervals that make up a full cycle 

bool blink_init(void);
bool blink_polling(void);

#endif