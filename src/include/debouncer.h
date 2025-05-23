#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include <stdbool.h>
#include <stdint.h>

// Função que verifica se o tempo decorrido desde a última interrupção
// é maior que o tempo definido para debouncing. Em caso afirmativo,
// atualiza o tempo e retorna true; caso contrário, retorna false.

bool check_debounce(uint32_t *last_interrupt_time, uint32_t debounce_time_us);

#endif
