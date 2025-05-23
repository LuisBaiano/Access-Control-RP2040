#include "debouncer.h"
#include "pico/stdlib.h"

/**
 * @brief Verifica se o tempo de debounce foi ultrapassado.
 *
 * Compara o tempo atual com o último tempo de interrupção e atualiza este
 * se o intervalo especificado em microssegundos tiver sido excedido.
 *
 * @param last_interrupt_time Ponteiro para a variável que armazena o tempo da última interrupção.
 * @param debounce_time_us Tempo de debounce em microssegundos.
 * @return true Se o intervalo de debounce foi ultrapassado.
 * @return false Se o intervalo de debounce ainda não foi atingido.
 */

bool check_debounce(uint32_t *last_interrupt_time, uint32_t debounce_time_us) {
    uint32_t current_time = time_us_32();
    if (current_time - *last_interrupt_time > debounce_time_us) {
        *last_interrupt_time = current_time;
        return true;
    }
    return false;
}
