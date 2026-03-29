#include "blink.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

uint8_t current_interval = LED_INTERVAL_TOTAL - 1;
unsigned long previous_ms = 0;
unsigned long current_ms = 0;

void blink(int led_pin){
    current_ms = to_ms_since_boot(get_absolute_time());

    if(current_ms - previous_ms >= LED_BASE_INTERVAL){
        previous_ms = current_ms;
        current_interval++;
    }

    switch(current_interval){
        case LED_INTERVAL_ON:
            gpio_put(PICO_DEFAULT_LED_PIN, 0);
        break;
        case LED_INTERVAL_TOTAL:
            gpio_put(PICO_DEFAULT_LED_PIN, 1);
            current_interval = 0;
        break; 
    }

    if (current_interval > LED_INTERVAL_TOTAL){
        current_interval = 0;
    }
}


bool blink_init(void){
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return 1;
}

bool blink_polling(void){
    blink(PICO_DEFAULT_LED_PIN);
}

