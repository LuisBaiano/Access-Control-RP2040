#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"
#include "lib/ssd1306/ssd1306.h"

void display_init(ssd1306_t *ssd); 
void display_startup_screen(ssd1306_t *ssd);
void display_update(ssd1306_t *ssd, uint8_t current_users, uint8_t max_users, const char* message);

#endif // DISPLAY_H