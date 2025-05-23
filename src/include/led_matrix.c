#include "led_matrix.h"
#include "config.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "led_matrix.pio.h" // Certifique-se que este nome está correto

#include <math.h>   // For fmaxf, fminf
#include <string.h> // For memset

// --- Internal Definitions ---
static PIO pio_instance = MATRIX_PIO_INSTANCE;
static uint pio_sm = MATRIX_PIO_SM;
static uint32_t pixel_buffer[MATRIX_SIZE];

#define MATRIX_GLOBAL_BRIGHTNESS 0.15f

typedef struct {
    float r;
    float g;
    float b;
} ws2812b_color_t;

static const ws2812b_color_t COLOR_BLACK       = {0.0f, 0.0f, 0.0f};
static const ws2812b_color_t COLOR_RED_ALERT   = {1.0f, 0.0f, 0.0f};
static const ws2812b_color_t COLOR_YELLOW_WATER= {0.8f, 0.8f, 0.0f};
static const ws2812b_color_t COLOR_BLUE_RAIN   = {0.0f, 0.3f, 1.0f};
static const ws2812b_color_t COLOR_ORANGE_BOTH = {1.0f, 0.4f, 0.0f};
static const ws2812b_color_t COLOR_GREEN_NORMAL= {0.0f, 1.0f, 0.0f};
static const ws2812b_color_t COLOR_CYAN_WATER_LEVEL = {0.0f, 0.7f, 0.7f};

static const uint8_t Leds_Matrix_position[MATRIX_DIM][MATRIX_DIM] = {
    {24, 23, 22, 21, 20},
    {15, 16, 17, 18, 19},
    {14, 13, 12, 11, 10},
    { 5,  6,  7,  8,  9 },
    { 4,  3,  2,  1,  0 }
};

// --- Frames de Alerta Definidos ---


static const char FRAME_RAIN[MATRIX_DIM][MATRIX_DIM + 1] = { // +1 para o terminador nulo '\0'
    "01010",
    "00100",
    "10101",
    "01010",
    "00100"
};

static const char FRAME_BOTH_ALERT[MATRIX_DIM][MATRIX_DIM + 1] = {
    "01010", // Chuva em cima
    "10101", // Chuva em cima
    "00100", // "Raio" ou separador
    "11111", // Nível água
    "11111"  // Nível água
};

static const char FRAME_NORMAL_STATUS[MATRIX_DIM][MATRIX_DIM + 1] = {
    "00000",
    "00001",
    "00010",
    "10100",
    "01000"
};


// --- Static Helper Functions ---
static inline int get_pixel_index(int row, int col) {
    // row e col são 0-based aqui para alinhar com arrays C
    if (row >= 0 && row < MATRIX_DIM && col >= 0 && col < MATRIX_DIM) {
        return Leds_Matrix_position[row][col];
    }
    return -1;
}

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

static void matrix_render() {
    for (int i = 0; i < MATRIX_SIZE; ++i) {
        pio_sm_put_blocking(pio_instance, pio_sm, pixel_buffer[i]);
    }
    busy_wait_us(50); // Para WS2812B, um delay de reset maior pode ser necessário (>50us, alguns recomendam 300us)
                      // mas o datasheet do SK6812 (similar) especifica >80us para reset.
                      // Se estiver usando SK6812 (comum nas matrizes do Pico), 50us pode ser pouco. Tente 100us ou 300us se tiver problemas.
}

// Set a pixel in the buffer usando row/col 0-based
static void led_activate_position_internal(int row, int col, ws2812b_color_t color, float brightness_mod) {
    int index = get_pixel_index(row, col);
    if (index != -1) {
        pixel_buffer[index] = color_to_pio_grb_format(color, MATRIX_GLOBAL_BRIGHTNESS * brightness_mod);
    }
}
// Sobrecarga com brilho padrão
static void led_activate_position(int row, int col, ws2812b_color_t color) {
    led_activate_position_internal(row, col, color, 1.0f);
}


// --- Public API Functions ---

void led_matrix_init() {
    uint offset = pio_add_program(pio_instance, &led_matrix_program); // Use o nome correto do programa .pio
    led_matrix_program_init(pio_instance, pio_sm, offset, MATRIX_WS2812_PIN); // Use o nome correto da função init
    led_matrix_clear(); // Limpa a matriz na inicialização
    printf("LED Matrix Initialized (Pin: %d, PIO: %d, SM: %d)\n", MATRIX_WS2812_PIN, pio_get_index(pio_instance), pio_sm);
}

void led_matrix_clear() {
    uint32_t pio_black = color_to_pio_grb_format(COLOR_BLACK, 1.0f);
    for (int i = 0; i < MATRIX_SIZE; ++i) {
        pixel_buffer[i] = pio_black;
    }
    // Não renderize aqui, deixe as funções de display fazerem o render
}

// Função auxiliar para desenhar um frame a partir de um array de char
static void draw_frame(const char frame[MATRIX_DIM][MATRIX_DIM + 1], ws2812b_color_t color) {
    for (int r = 0; r < MATRIX_DIM; ++r) {
        for (int c = 0; c < MATRIX_DIM; ++c) {
            if (frame[r][c] == '1') {
                led_activate_position(r, c, color);
            }
        }
    }
}
