#include "config.h"
#include "buttons.h"
#include "buzzer.h" 
#include "rgb_led.h"
#include "display.h"

// --- Definição dos Handles Globais ---
SemaphoreHandle_t xCountingSemaphoreUsers;
SemaphoreHandle_t xMutexDisplay;


ssd1306_t oled_display_global;

// --- Protótipos das Tarefas ---
void vTaskEntradaUsuarios(void *pvParameters);
void vTaskSaidaUsuarios(void *pvParameters);
void vTaskResetSistema(void *pvParameters);
void vTaskFeedbackVisualLedRgb(void *pvParameters);
void vTaskDisplayInfoOled(void *pvParameters);

// --- Inicialização do Sistema ---
void system_init_panel() {
    stdio_init_all();
    sleep_ms(1000);
    printf("Initializing Interactive Control Panel...\n");

    buttons_init();
    buzzer_init(); // Apenas inicializa o pino do buzzer
    rgb_led_init();
    display_init(&oled_display_global);

    printf("Hardware Initialized.\n");
}

// --- Função Principal ---
int main() {
    system_init_panel();

    xCountingSemaphoreUsers = xSemaphoreCreateCounting(MAX_USERS, MAX_USERS);
    xMutexDisplay = xSemaphoreCreateMutex();

    if (xCountingSemaphoreUsers == NULL || xMutexDisplay == NULL) {
        printf("FATAL: Failed to create semaphore/mutex!\n");
        while(1);
    }
    printf("Counting Semaphore and Mutex created.\n");

    if (xSemaphoreTake(xMutexDisplay, portMAX_DELAY) == pdTRUE) {
        ssd1306_fill(&oled_display_global, false);
        ssd1306_draw_string(&oled_display_global, "Painel Controle", 5, 10);
        ssd1306_draw_string(&oled_display_global, "Iniciando...", 5, 25);
        ssd1306_send_data(&oled_display_global);
        xSemaphoreGive(xMutexDisplay);
    }
    sleep_ms(2000);

    printf("Creating tasks...\n");
    xTaskCreate(vTaskEntradaUsuarios, "EntradaUser", STACK_SIZE_DEFAULT, NULL, PRIORITY_ENTRADA_USUARIO, NULL);
    xTaskCreate(vTaskSaidaUsuarios, "SaidaUser", STACK_SIZE_DEFAULT, NULL, PRIORITY_SAIDA_USUARIO, NULL);
    xTaskCreate(vTaskResetSistema, "ResetSystem", STACK_SIZE_DEFAULT, NULL, PRIORITY_RESET_SYSTEM, NULL);
    xTaskCreate(vTaskFeedbackVisualLedRgb, "LedFeedback", STACK_SIZE_DEFAULT, NULL, PRIORITY_FEEDBACK_RGB, NULL);
    xTaskCreate(vTaskDisplayInfoOled, "DisplayInfo", STACK_SIZE_DISPLAY, &oled_display_global, PRIORITY_DISPLAY_INFO, NULL);

    printf("Starting FreeRTOS scheduler...\n");
    vTaskStartScheduler();

    printf("Scheduler returned. Error!\n");
    while(1);
}


// --- Implementações das Tarefas ---

void vTaskEntradaUsuarios(void *pvParameters) {
    printf("Task Entrada Usuarios started.\n");
    char display_msg[30] = "";

    while (true) {
        if (buttons_a_pressed()) {
            printf("Botao A pressionado - Tentando entrada.\n");
            if (xSemaphoreTake(xCountingSemaphoreUsers, pdMS_TO_TICKS(50)) == pdTRUE) {
                uint32_t vagas_atuais = uxSemaphoreGetCount(xCountingSemaphoreUsers);
                uint8_t usuarios_ativos = MAX_USERS - vagas_atuais;
                printf("Entrada OK! Usuarios: %u, Vagas: %lu\n", usuarios_ativos, vagas_atuais);
                sprintf(display_msg, "Entrada OK (%u/%u)", usuarios_ativos, MAX_USERS);
            } else {
                printf("Capacidade Maxima Atingida!\n");
                buzzer_play_tone(BUZZER_BEEP_SHORT_FREQ, BUZZER_BEEP_SHORT_MS); // Chamada direta
                strcpy(display_msg, "Lotado!");
            }
            if (xSemaphoreTake(xMutexDisplay, portMAX_DELAY) == pdTRUE) {
                uint32_t vagas = uxSemaphoreGetCount(xCountingSemaphoreUsers);
                display_update_panel(&oled_display_global, MAX_USERS - vagas, MAX_USERS, display_msg);
                xSemaphoreGive(xMutexDisplay);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_DELAY_MS));
    }
}

void vTaskSaidaUsuarios(void *pvParameters) {
    printf("Task Saida Usuarios started.\n");
    char display_msg[30] = "";

    while (true) {
        if (buttons_b_pressed()) {
            printf("Botao B pressionado - Tentando saida.\n");
            if (uxSemaphoreGetCount(xCountingSemaphoreUsers) < MAX_USERS) {
                if (xSemaphoreGive(xCountingSemaphoreUsers) == pdTRUE) {
                    uint32_t vagas_atuais = uxSemaphoreGetCount(xCountingSemaphoreUsers);
                    uint8_t usuarios_ativos = MAX_USERS - vagas_atuais;
                    printf("Saida OK! Usuarios: %u, Vagas: %lu\n", usuarios_ativos, vagas_atuais);
                    sprintf(display_msg, "Saida OK (%u/%u)", usuarios_ativos, MAX_USERS);
                } else {
                    strcpy(display_msg, "Erro Saida");
                }
            } else {
                strcpy(display_msg, "Vazio");
            }
            if (xSemaphoreTake(xMutexDisplay, portMAX_DELAY) == pdTRUE) {
                uint32_t vagas = uxSemaphoreGetCount(xCountingSemaphoreUsers);
                display_update_panel(&oled_display_global, MAX_USERS - vagas, MAX_USERS, display_msg);
                xSemaphoreGive(xMutexDisplay);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_DELAY_MS));
    }
}

void vTaskResetSistema(void *pvParameters) {
    printf("Task Reset Sistema started. Esperando pelo botao joystick...\n");
    char display_msg[30] = "";

    while (true) {
        if (buttons_joystick_pressed()) {
            printf("BOTAO JOYSTICK PRESSIONADO - RESET SOLICITADO!\n");

            buzzer_play_tone(BUZZER_BEEP_RESET_FREQ, BUZZER_BEEP_RESET_ON_MS);
            vTaskDelay(pdMS_TO_TICKS(BUZZER_BEEP_RESET_OFF_MS));
            buzzer_play_tone(BUZZER_BEEP_RESET_FREQ, BUZZER_BEEP_RESET_ON_MS);

            printf("Resetando contagem de usuarios...\n");
            taskENTER_CRITICAL();
            while (uxSemaphoreGetCount(xCountingSemaphoreUsers) < MAX_USERS) {
                if (xSemaphoreGive(xCountingSemaphoreUsers) != pdTRUE) {
                    printf("Erro critico ao dar give no semaforo de contagem durante o reset!\n");
                    break;
                }
            }
            taskEXIT_CRITICAL();

            uint32_t vagas_apos_reset = uxSemaphoreGetCount(xCountingSemaphoreUsers);
            printf("Sistema Resetado! Vagas: %lu / %u\n", vagas_apos_reset, MAX_USERS);
            strcpy(display_msg, "Sistema Resetado");

            if (xSemaphoreTake(xMutexDisplay, portMAX_DELAY) == pdTRUE) {
                display_update_panel(&oled_display_global, 0, MAX_USERS, display_msg);
                xSemaphoreGive(xMutexDisplay);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_DELAY_MS));
    }
}

// vTaskFeedbackVisualLedRgb
void vTaskFeedbackVisualLedRgb(void *pvParameters) {
    printf("Task Feedback LED RGB started.\n");
    uint8_t usuarios_ativos;
    uint32_t vagas_atuais_disponiveis;

    while (true) {
        vagas_atuais_disponiveis = uxSemaphoreGetCount(xCountingSemaphoreUsers);
        usuarios_ativos = MAX_USERS - vagas_atuais_disponiveis;
        rgb_led_set(usuarios_ativos, MAX_USERS);
        vTaskDelay(pdMS_TO_TICKS(RGB_UPDATE_DELAY_MS));
    }
}

// vTaskDisplayInfoOled
void vTaskDisplayInfoOled(void *pvParameters) {
    ssd1306_t *disp = (ssd1306_t *)pvParameters;
    printf("Task Display Info OLED started.\n");
    // char temp_msg_buffer[32];

    while (true) {
        uint32_t vagas_atuais_disponiveis;
        uint8_t usuarios_ativos;

        vagas_atuais_disponiveis = uxSemaphoreGetCount(xCountingSemaphoreUsers);
        usuarios_ativos = MAX_USERS - vagas_atuais_disponiveis;

        if (xSemaphoreTake(xMutexDisplay, portMAX_DELAY) == pdTRUE) {
            // Passa NULL para a mensagem para que display_update_panel use sua lógica interna de mensagem
            display_update_panel(disp, usuarios_ativos, MAX_USERS, NULL);
            xSemaphoreGive(xMutexDisplay);
        }
        vTaskDelay(pdMS_TO_TICKS(DISPLAY_UPDATE_DELAY_MS));
    }
}