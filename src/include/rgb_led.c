#include "config.h" 
#include "hardware/gpio.h"
#include <stdint.h>
#include "rgb_led.h" 


/**
 * @brief Inicializa os pinos do LED RGB como saídas digitais.
 *        Define os LEDs inicialmente como desligados.
 */
void rgb_led_init() {
    gpio_init(LED_RED_PIN);
    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_put(LED_RED_PIN, 0); // Desligado

    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_put(LED_GREEN_PIN, 0); // Desligado

    gpio_init(LED_BLUE_PIN);
    gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);
    gpio_put(LED_BLUE_PIN, 0); // Desligado

    printf("LED RGB (GPIO) inicializado.\n");
}

void rgb_led_set(uint8_t actual_num_users, uint8_t max_capacity) {
    if (actual_num_users == 0) {
        // Nenhum usuário logado: Azul
        gpio_put(LED_RED_PIN, 0); gpio_put(LED_GREEN_PIN, 0); gpio_put(LED_BLUE_PIN, 1);
    } else if (actual_num_users == max_capacity) {
        // Capacidade máxima: Vermelho
        gpio_put(LED_RED_PIN, 1); gpio_put(LED_GREEN_PIN, 0); gpio_put(LED_BLUE_PIN, 0);
    } else if (actual_num_users == max_capacity - 1) {
        // Apenas 1 vaga restante: Amarelo (R+G)
        gpio_put(LED_RED_PIN, 1); gpio_put(LED_GREEN_PIN, 1); gpio_put(LED_BLUE_PIN, 0);
    } else { // Usuários ativos (de 1 a MAX-2)
        // Verde
        gpio_put(LED_RED_PIN, 0); gpio_put(LED_GREEN_PIN, 1); gpio_put(LED_BLUE_PIN, 0);
    }
}