#include "buttons.h"
#include "config.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "debouncer.h"

static volatile bool flag_button_a = false;
static volatile bool flag_button_b = false;
static volatile bool flag_joy_button = false;

static uint32_t last_time_a = 0;
static uint32_t last_time_b = 0;
static uint32_t last_time_joy = 0;

/**
 * @brief Callback interno de interrupção para todos os botões.
 *
 * Essa função é chamada quando ocorre uma interrupção nos pinos dos botões.
 * Ela verifica qual botão foi pressionado (borda de descida) e define uma flag,
 * respeitando o tempo de debounce.
 *
 * @param gpio Número do pino GPIO que gerou a interrupção.
 * @param events Máscara com os eventos ocorridos (ex: GPIO_IRQ_EDGE_FALL).
 */
static void gpio_callback_internal(uint gpio, uint32_t events) {
    if (events & GPIO_IRQ_EDGE_FALL) {
        if (gpio == BUTTON_A_PIN) {
            if (check_debounce(&last_time_a, DEBOUNCE_TIME_US)) {
                flag_button_a = true;
            }
        } else if (gpio == BUTTON_B_PIN) {
            if (check_debounce(&last_time_b, DEBOUNCE_TIME_US)) {
                flag_button_b = true;
            }
        } else if (gpio == JOYSTICK_BTN_PIN) {
            if (check_debounce(&last_time_joy, DEBOUNCE_TIME_US)) {
                flag_joy_button = true;
            }
        }
    }
}

/**
 * @brief Inicializa os botões e configura suas interrupções.
 *
 * Configura os pinos dos botões como entradas com pull-up e ativa as interrupções
 * na borda de descida com o callback interno.
 */
void buttons_init() {
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);

    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN);

    gpio_init(JOYSTICK_BTN_PIN);
    gpio_set_dir(JOYSTICK_BTN_PIN, GPIO_IN);
    gpio_pull_up(JOYSTICK_BTN_PIN);

    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback_internal);
    gpio_set_irq_enabled_with_callback(BUTTON_B_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback_internal);
    gpio_set_irq_enabled_with_callback(JOYSTICK_BTN_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback_internal);
}

/**
 * @brief Verifica se o botão foi pressionado.
 *
 * @return true se o botão foi pressionado desde a última chamada. Caso contrário, false.
 */

bool buttons_a_pressed() {
    if (flag_button_a) {
        flag_button_a = false;
        return true;
    }
    return false;
}

bool buttons_b_pressed() {
    if (flag_button_b) {
        flag_button_b = false;
        return true;
    }
    return false;
}

bool buttons_joystick_pressed() {
    if (flag_joy_button) {
        flag_joy_button = false;
        return true;
    }
    return false;
}
