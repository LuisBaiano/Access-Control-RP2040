#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// FreeRTOS includes
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h" // Para Semáforos e Mutex
#include "queue.h" 
#include "FreeRTOSConfig.h"

// --- Constantes do Sistema ---
#define MAX_USERS 5 // Capacidade máxima do espaço (ex: 5 para facilitar teste)

// --- Definições de Pinos ---
#define BUTTON_A_PIN        5  // Entrada de usuário
#define BUTTON_B_PIN        6  // Saída de usuário
#define JOYSTICK_BTN_PIN    22 // Reset do sistema

// LED RGB
#define LED_RED_PIN         13
#define LED_GREEN_PIN       11
#define LED_BLUE_PIN        12

// Matriz de LEDs
#define MATRIX_WS2812_PIN 7
#define MATRIX_SIZE       25
#define MATRIX_DIM        5
#define MATRIX_PIO_INSTANCE pio0
#define MATRIX_PIO_SM       0

// Buzzer
#define BUZZER_PIN_MAIN     10

// Display OLED
#define I2C_PORT        i2c1
#define I2C_SDA_PIN     14
#define I2C_SCL_PIN     15
#define DISPLAY_ADDR    0x3C
#define DISPLAY_WIDTH   128
#define DISPLAY_HEIGHT  64

// --- Constantes de Tempo e Comportamento ---
#define DEBOUNCE_TIME_US         50000 // 50ms para botões

// Buzzer
#define BUZZER_BEEP_SHORT_FREQ    1000
#define BUZZER_BEEP_SHORT_MS      100
#define BUZZER_BEEP_RESET_FREQ    1500
#define BUZZER_BEEP_RESET_ON_MS   150
#define BUZZER_BEEP_RESET_OFF_MS  100

// Delays das Tarefas (ms)
#define BUTTON_POLL_DELAY_MS      50 // Se tarefas de botão fizerem polling
#define RGB_UPDATE_DELAY_MS       200
#define DISPLAY_UPDATE_DELAY_MS   500
#define RESET_TASK_CHECK_DELAY_MS 10 // Para loop de reset do semáforo

// --- Configuração de Tarefas FreeRTOS ---
// Prioridades
#define PRIORITY_RESET_SYSTEM     (tskIDLE_PRIORITY + 4) // Alta para responder ao reset
#define PRIORITY_ENTRADA_USUARIO  (tskIDLE_PRIORITY + 3)
#define PRIORITY_SAIDA_USUARIO    (tskIDLE_PRIORITY + 3)
#define PRIORITY_FEEDBACK_RGB     (tskIDLE_PRIORITY + 1)
#define PRIORITY_DISPLAY_INFO     (tskIDLE_PRIORITY + 0) // Mais baixa

// Tamanho das Stacks (configMINIMAL_STACK_SIZE é definido em FreeRTOSConfig.h)
#define STACK_MULTIPLIER_DEFAULT  2
#define STACK_MULTIPLIER_DISPLAY  4
#define STACK_SIZE_DEFAULT        (configMINIMAL_STACK_SIZE * STACK_MULTIPLIER_DEFAULT)
#define STACK_SIZE_DISPLAY        (configMINIMAL_STACK_SIZE * STACK_MULTIPLIER_DISPLAY)


// --- Handles para Semáforos e Mutex (Declarações Externas) ---
extern SemaphoreHandle_t xCountingSemaphoreUsers;
extern SemaphoreHandle_t xMutexDisplay;

#endif // HARDWARE_CONFIG_H