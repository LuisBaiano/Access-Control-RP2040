#ifndef RGB_LED_H
#define RGB_LED_H

#include "pico/stdlib.h"
#include "config.h" 
#include <stdint.h>

void rgb_led_init(void);
void rgb_led_set(uint8_t actual_num_users, uint8_t max_capacity);

#endif // RGB_LED_H