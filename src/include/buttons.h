#ifndef BUTTONS_H
#define BUTTONS_H

#include "pico/stdlib.h"
#include <stdbool.h>

// Inicializa os botões e configura as interrupções
void buttons_init();

// Verifica se o botão foi pressionado desde a última verificação
bool buttons_a_pressed();
bool buttons_b_pressed();
bool buttons_joystick_pressed();

#endif