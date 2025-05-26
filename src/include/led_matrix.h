#ifndef LED_MATRIX_H
#define LED_MATRIX_H

#include <stdint.h>
typedef enum {
    MATRIX_STATE_VAZIO,         // Livre
    MATRIX_STATE_VAGAS_LIVRES,  // Ocupação média
    MATRIX_STATE_QUASE_CHEIO,   // Quase cheio
    MATRIX_STATE_CHEIO,          // Lotado
} MatrixOccupationState_t;

void led_matrix_init(void);
void led_matrix_clear(void);
void led_matrix_ocupacao(MatrixOccupationState_t state, uint8_t animation_step);

#endif // LED_MATRIX_H