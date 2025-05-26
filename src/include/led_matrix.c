#include "led_matrix.h"
#include "config.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "led_matrix.pio.h"

#include <math.h>
#include <string.h>

static PIO pio_instance = MATRIX_PIO_INSTANCE;
static uint pio_sm = MATRIX_PIO_SM;
static uint32_t pixel_buffer[MATRIX_SIZE];

#define MATRIX_GLOBAL_BRIGHTNESS 0.2f 

/** 
 * @struct ws2812b_color_t
 * @brief Representa uma cor RGB em ponto flutuante (0.0 a 1.0).
 */
typedef struct { float r; float g; float b; } ws2812b_color_t;

// Definições de cores utilizadas para diferentes estados de ocupação
static const ws2812b_color_t COLOR_BLACK        = {0.0f, 0.0f, 0.0f};
static const ws2812b_color_t COLOR_BLUE_EMPTY   = {0.1f, 0.1f, 1.0f};
static const ws2812b_color_t COLOR_GREEN_GO     = {0.0f, 1.0f, 0.1f};
static const ws2812b_color_t COLOR_YELLOW_WARN  = {1.0f, 0.8f, 0.0f};
static const ws2812b_color_t COLOR_RED_FULL     = {1.0f, 0.0f, 0.0f};
static const ws2812b_color_t COLOR_WHITE_RESET  = {1.0f, 1.0f, 1.0f};

// Mapeamento físico da matriz de LEDs para a lógica do programa
static const uint8_t Leds_Matrix_position[MATRIX_DIM][MATRIX_DIM] = {
    {24, 23, 22, 21, 20},
    {15, 16, 17, 18, 19},
    {14, 13, 12, 11, 10},
    { 5,  6,  7,  8,  9 },
    { 4,  3,  2,  1,  0 }
};

// Frames de ícones representados em formato binário
static const char FRAME_EMPTY_ICON[MATRIX_DIM][MATRIX_DIM + 1] = {
    "01110", "10001", "10001", "10001", "01110"
};
static const char FRAME_NORMAL_ICON[MATRIX_DIM][MATRIX_DIM + 1] = {
    "00000", "00001", "00010", "10100", "01000"
};
static const char FRAME_WARN_ICON[MATRIX_DIM][MATRIX_DIM + 1] = {
    "00100", "00100", "00100", "00000", "00100"
};
static const char FRAME_FULL_ICON[MATRIX_DIM][MATRIX_DIM + 1] = {
    "10001", "01010", "00100", "01010", "10001"
};
static const char FRAME_CIRCLE_SMALL[MATRIX_DIM][MATRIX_DIM + 1] = {
    "00000", "00100", "01010", "00100", "00000"
};

/**
 * @brief Obtém o índice no buffer para uma posição na matriz (linha, coluna).
 * @param row Linha (0 a MATRIX_DIM - 1)
 * @param col Coluna (0 a MATRIX_DIM - 1)
 * @return Índice do pixel no buffer ou -1 se fora dos limites.
 */
static inline int get_pixel_index(int row, int col) {
    if (row >= 0 && row < MATRIX_DIM && col >= 0 && col < MATRIX_DIM) {
        return Leds_Matrix_position[row][col];
    }
    return -1;
}

/**
 * @brief Converte uma cor RGB para o formato GRB usado pelo protocolo WS2812b.
 * @param color Cor no formato RGB flutuante.
 * @param brightness Fator de brilho (0.0 a 1.0)
 * @return Valor de 32 bits no formato esperado pelo WS2812b.
 */
static inline uint32_t color_to_pio_grb_format(ws2812b_color_t color, float brightness) {
    brightness = fmaxf(0.0f, fminf(1.0f, brightness));
    float r = fmaxf(0.0f, fminf(1.0f, color.r * brightness));
    float g = fmaxf(0.0f, fminf(1.0f, color.g * brightness)); 
    float b = fmaxf(0.0f, fminf(1.0f, color.b * brightness));

    uint8_t R_val = (uint8_t)(r * 255.0f + 0.3f); 
    uint8_t G_val = (uint8_t)(g * 255.0f + 0.3f);
    uint8_t B_val = (uint8_t)(b * 255.0f + 0.3f);
    return ((uint32_t)(G_val) << 24) | ((uint32_t)(R_val) << 16) | ((uint32_t)(B_val) << 8);
}

/**
 * @brief Atualiza a matriz enviando o buffer atual para o PIO.
 */
static void matrix_update() {
    for (int i = 0; i < MATRIX_SIZE; ++i) {
        pio_sm_put_blocking(pio_instance, pio_sm, pixel_buffer[i]);
    }
    busy_wait_us(300);
}

/**
 * @brief Ativa um pixel individual na matriz com cor e brilho ajustado.
 * @param row Linha do pixel
 * @param col Coluna do pixel
 * @param color Cor desejada
 * @param brightness_modulator Modificador adicional ao brilho global
 */
static void led_activate_position(int row_0_based, int col_0_based, ws2812b_color_t color, float brightness_modulator) {
    int index = get_pixel_index(row_0_based, col_0_based);
    if (index != -1) {
        pixel_buffer[index] = color_to_pio_grb_format(color, MATRIX_GLOBAL_BRIGHTNESS * brightness_modulator);
    }
}

/**
 * @brief Inicializa o PIO e o estado inicial da matriz.
 */
void led_matrix_init() {
    uint offset = pio_add_program(pio_instance, &led_matrix_program);
    led_matrix_program_init(pio_instance, pio_sm, offset, MATRIX_WS2812_PIN);
    led_matrix_clear();
}

/**
 * @brief Limpa todos os pixels da matriz (define como preto) e atualiza a exibição.
 */
void led_matrix_clear() {
    for (int i = 0; i < MATRIX_SIZE; ++i) {
        pixel_buffer[i] = color_to_pio_grb_format(COLOR_BLACK, 1.0f);
    }
    matrix_update();
}

/**
 * @brief Preenche toda a matriz com uma cor uniforme, com brilho modulado.
 * @param color Cor base
 * @param brightness_modulator Fator de modulação do brilho
 */
static void pulsa_leds(ws2812b_color_t color, float brightness_modulator) {
    uint32_t pio_fill_color = color_to_pio_grb_format(color, MATRIX_GLOBAL_BRIGHTNESS * brightness_modulator);
    for (int i = 0; i < MATRIX_SIZE; ++i) {
        pixel_buffer[i] = pio_fill_color;
    }
    matrix_update();
}

/**
 * @brief Desenha um frame binário na matriz com a cor e brilho especificados.
 * @param frame Frame de 5x5 representando a imagem
 * @param base_color Cor base
 * @param brightness_modulator Brilho ajustável
 */
static void desenha_frame(const char frame[MATRIX_DIM][MATRIX_DIM + 1], ws2812b_color_t base_color, float brightness_modulator) {
    for (int r = 0; r < MATRIX_DIM; ++r) {
        for (int c = 0; c < MATRIX_DIM; ++c) {
            if (frame[r][c] == '1') {
                led_activate_position(r, c, base_color, brightness_modulator);
            }
        }
    }
}

/**
 * @brief Atualiza a exibição da matriz de LEDs com base no estado atual e passo de animação.
 * 
 * @param state Estado lógico atual da matriz (vazio, normal, cheio, etc.)
 * @param animation_step Passo atual da animação (incrementado periodicamente)
 */
void led_matrix_ocupacao(MatrixOccupationState_t state, uint8_t animation_step) {
    for (int i = 0; i < MATRIX_SIZE; ++i) {
        pixel_buffer[i] = color_to_pio_grb_format(COLOR_BLACK, 1.0f);
    }

    float angle = (float)animation_step * (2.0f * (float)M_PI / 100.0f);
    float sine_wave_0_to_1 = (sinf(angle) + 1.0f) / 2.0f;

    float min_brightness_factor_for_pulse = 0.2f;
    float current_pulse_brightness_factor = min_brightness_factor_for_pulse +
                                           ((1.0f - min_brightness_factor_for_pulse) * sine_wave_0_to_1);

    const char (*selected_frame_ptr)[MATRIX_DIM+1] = NULL;
    ws2812b_color_t selected_color = COLOR_BLACK;

    bool apply_standard_pulsing = true;

    switch (state) {
        case MATRIX_STATE_VAZIO:
            if ((animation_step / 15) % 2 == 0) {
                selected_frame_ptr = FRAME_EMPTY_ICON;
            } else {
                selected_frame_ptr = FRAME_CIRCLE_SMALL;
            }
            selected_color = COLOR_BLUE_EMPTY;
            break;

        case MATRIX_STATE_VAGAS_LIVRES:
            selected_frame_ptr = FRAME_NORMAL_ICON;
            selected_color = COLOR_GREEN_GO;
            break;

        case MATRIX_STATE_QUASE_CHEIO:
            apply_standard_pulsing = false;
            if ((animation_step / 10) % 2 == 0) {
                desenha_frame(FRAME_WARN_ICON, COLOR_YELLOW_WARN, 1.0f);
            }
            break;

        case MATRIX_STATE_CHEIO:
            selected_frame_ptr = FRAME_FULL_ICON;
            selected_color = COLOR_RED_FULL;
            break;

        default:
            selected_frame_ptr = NULL;
            apply_standard_pulsing = false;
            break;
    }

    if (selected_frame_ptr && apply_standard_pulsing) {
        desenha_frame(selected_frame_ptr, selected_color, current_pulse_brightness_factor);
    }

    matrix_update();
}