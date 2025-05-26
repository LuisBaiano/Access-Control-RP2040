// src/main.c

#include "config.h"      // Assume que este é o seu hardware_config.h principal
#include "buttons.h"     // Funções de botões
#include "buzzer.h"      // Funções do buzzer
#include "rgb_led.h"     // Funções do LED RGB
#include "display.h"     // Funções do display OLED
#include "led_matrix.h"  // Funções da matriz de LEDs

// --- Definição dos Handles Globais ---
// Os handles são declarados como extern em config.h e definidos aqui.
SemaphoreHandle_t xCountingSemaphoreUsers;
SemaphoreHandle_t xMutexDisplay;

ssd1306_t ssd; // Objeto global para o display OLED

// --- Protótipos das Tarefas ---
void vTaskEntradaUsuarios(void *pvParameters);
void vTaskSaidaUsuarios(void *pvParameters);
void vTaskResetSistema(void *pvParameters);
void vTaskFeedbackVisualLedRgb(void *pvParameters);
void vTaskDisplayInfoOled(void *pvParameters);
void vTaskLedMatrixControl(void *pvParameters);

// --- Inicialização do Sistema ---
/**
 * @brief Inicializa todos os periféricos e subsistemas necessários para o painel de controle.
 * Configura STDIO, botões, buzzer, LED RGB, matriz de LEDs e o display OLED.
 * Exibe uma tela de inicialização no display.
 */
void system_init_panel() {
    stdio_init_all();
    sleep_ms(1000); // Aguarda estabilização do terminal serial

    buttons_init();     // Inicializa botões e suas interrupções/flags
    buzzer_init();      // Inicializa o pino do buzzer
    rgb_led_init();     // Inicializa os pinos do LED RGB
    led_matrix_init();  // Inicializa o PIO e a matriz de LEDs
    display_init(&ssd); // Inicializa o I2C e o controlador do display OLED
}

// --- Função Principal ---
/**
 * @brief Ponto de entrada do programa.
 * Inicializa o sistema, cria os semáforos e mutexes do FreeRTOS,
 * exibe uma tela de inicialização, cria as tarefas da aplicação e
 * inicia o escalonador do FreeRTOS.
 *
 * @return int Código de retorno.
 */
int main() {
    system_init_panel(); // Inicializa todo o hardware

    // Cria o semáforo de contagem para controlar as vagas
    // MAX_USERS é o número máximo de "vagas" que o semáforo pode contar
    // MAX_USERS é também a contagem inicial, significando que todas as vagas estão disponíveis
    xCountingSemaphoreUsers = xSemaphoreCreateCounting(MAX_USERS, MAX_USERS);

    // Cria o mutex para proteger o acesso ao display OLED
    xMutexDisplay = xSemaphoreCreateMutex();

    if (xCountingSemaphoreUsers == NULL || xMutexDisplay == NULL) {
        printf("FATAL: Failed to create semaphore/mutex!\n");
        while(1); // Trava se a criação falhar
    }
    printf("mutex e contador iniciados.\n");

    // Exibe a tela de startup no display APÓS criar o mutex
    display_startup_screen(&ssd);


    printf("Creating tasks...\n");
    // Cria as tarefas da aplicação, passando prioridades e tamanhos de stack definidos em config.h
    xTaskCreate(vTaskEntradaUsuarios, "EntradaUser", STACK_SIZE_DEFAULT, NULL, PRIORITY_ENTRADA_USUARIO, NULL);
    xTaskCreate(vTaskSaidaUsuarios, "SaidaUser", STACK_SIZE_DEFAULT, NULL, PRIORITY_SAIDA_USUARIO, NULL);
    xTaskCreate(vTaskResetSistema, "ResetSystem", STACK_SIZE_DEFAULT, NULL, PRIORITY_RESET_SYSTEM, NULL);
    xTaskCreate(vTaskFeedbackVisualLedRgb, "LedFeedback", STACK_SIZE_DEFAULT, NULL, PRIORITY_FEEDBACK_RGB, NULL);
    xTaskCreate(vTaskDisplayInfoOled, "DisplayInfo", STACK_SIZE_DISPLAY, &ssd, PRIORITY_DISPLAY_INFO, NULL);
    xTaskCreate(vTaskLedMatrixControl, "MatrixCtrl", STACK_SIZE_DEFAULT, NULL, PRIORITY_MATRIX, NULL); // Prioridade da matriz

    printf("Inicializacao do FreeRTOS...\n"); // Corrigido para "Inicialização"
    vTaskStartScheduler(); // Inicia o escalonador do FreeRTOS

    // O programa nunca deve chegar a este ponto
    printf("Scheduler returned. Error!\n");
    while(1);
}

// --- Implementações das Tarefas ---

/**
 * @brief Tarefa responsável por processar a entrada de usuários.
 * Monitora o Botão A. Se pressionado, tenta "ocupar" uma vaga decrementando
 * o semáforo de contagem. Se o espaço estiver lotado (semáforo não pôde ser tomado),
 * emite um beep de aviso. Atualiza o display OLED com o status da operação e
 * a contagem atual de usuários/vagas, protegendo o acesso com um mutex.
 */
void vTaskEntradaUsuarios(void *pvParameters) {
    printf("Task da Entrada de Usuarios iniciada.\n");
    char display_msg[30] = "";

    while (true) {
        if (buttons_a_pressed()) { // Verifica se o Botão A foi acionado
            printf("Botao A pressionado - Tentando entrada.\n");
            // Tenta tomar uma "vaga" do semáforo (decrementar a contagem de vagas disponíveis)
            if (xSemaphoreTake(xCountingSemaphoreUsers, pdMS_TO_TICKS(50)) == pdTRUE) {
                // Sucesso: vaga ocupada
                uint32_t vagas_atuais = uxSemaphoreGetCount(xCountingSemaphoreUsers);
                uint8_t usuarios_ativos = MAX_USERS - vagas_atuais;
                printf("Entrada OK! Usuarios: %u, Vagas: %lu\n", usuarios_ativos, vagas_atuais);
                sprintf(display_msg, "Entrada (%u/%u)", usuarios_ativos, MAX_USERS);
            } else {
                // Falha: semáforo em 0 (sem vagas) - capacidade máxima atingida
                printf("Capacidade Maxima Atingida!\n");
                buzzer_play_tone(BUZZER_BEEP_SHORT_FREQ, BUZZER_BEEP_SHORT_MS); // Beep de lotado
                strcpy(display_msg, "Lotado!");
            }

            // Atualiza o display (operação protegida por mutex)
            if (xSemaphoreTake(xMutexDisplay, portMAX_DELAY) == pdTRUE) {
                uint32_t vagas = uxSemaphoreGetCount(xCountingSemaphoreUsers); 
                display_update(&ssd, MAX_USERS - vagas, MAX_USERS, display_msg);
                xSemaphoreGive(xMutexDisplay);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_DELAY_MS)); // Pausa para polling do botão
    }
}

/**
 * @brief Tarefa responsável por processar a saída de usuários.
 * Monitora o Botão B. Se pressionado e houver usuários no espaço
 * (ou seja, o semáforo de vagas não está no máximo), "libera" uma vaga
 * incrementando o semáforo de contagem. Atualiza o display OLED
 * com o status e a contagem, protegendo o acesso com um mutex.
 */
void vTaskSaidaUsuarios(void *pvParameters) {
    printf("Task Saida Usuarios iniciada.\n");
    char display_msg[30] = "";

    while (true) {
        if (buttons_b_pressed()) { // Verifica se o Botão B foi acionado
            printf("Botao B pressionado - Tentando saida.\n");
            // Verifica se há usuários para sair (ou seja, se nem todas as vagas estão disponíveis)
            if (uxSemaphoreGetCount(xCountingSemaphoreUsers) < MAX_USERS) {
                // Libera uma vaga, incrementando a contagem de vagas disponíveis no semáforo
                if (xSemaphoreGive(xCountingSemaphoreUsers) == pdTRUE) {
                    // Sucesso: vaga liberada
                    uint32_t vagas_atuais = uxSemaphoreGetCount(xCountingSemaphoreUsers);
                    uint8_t usuarios_ativos = MAX_USERS - vagas_atuais;
                    printf("Saida OK! Usuarios: %u, Vagas: %lu\n", usuarios_ativos, vagas_atuais);
                    sprintf(display_msg, "Saida (%u/%u)", usuarios_ativos, MAX_USERS);
                } else {
                    // Esta condição (falha ao dar give em semáforo de contagem abaixo do max)
                    // não deveria ocorrer se a lógica estiver correta.
                    strcpy(display_msg, "Erro Saida!");
                }
            } else {
                // Todas as vagas já estão disponíveis (ninguém para sair)
                printf("Espaco Vazio. Ninguem para sair.\n");
                strcpy(display_msg, "Vazio");
            }

            // Atualiza o display (operação protegida por mutex)
            if (xSemaphoreTake(xMutexDisplay, portMAX_DELAY) == pdTRUE) {
                uint32_t vagas = uxSemaphoreGetCount(xCountingSemaphoreUsers); // Re-lê para consistência
                display_update(&ssd, MAX_USERS - vagas, MAX_USERS, display_msg); // Assumindo que display_update é sua função
                xSemaphoreGive(xMutexDisplay);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_DELAY_MS)); // Pausa para polling do botão
    }
}

/**
 * @brief Tarefa responsável por resetar o sistema de contagem de usuários.
 * Monitora o botão do joystick (via `buttons_joystick_pressed()`, que consome
 * uma flag setada pela ISR). Se pressionado, emite um beep duplo de confirmação
 * e restaura o semáforo de contagem para seu estado inicial (todas as vagas
 * disponíveis), iterando `xSemaphoreGive`. Atualiza o display OLED,
 * protegendo o acesso com um mutex.
 */
void vTaskResetSistema(void *pvParameters) {
    printf("Task Reset Sistema iniciada.\n");
    char display_msg[30] = "";

    while (true) {
        if (buttons_joystick_pressed()) { // Verifica se o botão do joystick foi acionado
            printf("Botao Joystick pressionado - RESET SOLICITADO!\n");

            // Emite beep duplo de reset
            buzzer_play_tone(BUZZER_BEEP_RESET_FREQ, BUZZER_BEEP_RESET_ON_MS);
            vTaskDelay(pdMS_TO_TICKS(BUZZER_BEEP_RESET_OFF_MS)); // Pausa entre beeps
            buzzer_play_tone(BUZZER_BEEP_RESET_FREQ, BUZZER_BEEP_RESET_ON_MS);

            printf("Resetando contagem de usuarios...\n");
            // Entra em seção crítica para garantir que a manipulação do semáforo seja atômica
            // em relação a outras tarefas que possam tentar usá-lo (embora aqui o objetivo seja "encher" as vagas).
            taskENTER_CRITICAL();
            // Libera todas as vagas dando "give" no semáforo até ele atingir MAX_USERS
            while (uxSemaphoreGetCount(xCountingSemaphoreUsers) < MAX_USERS) {
                if (xSemaphoreGive(xCountingSemaphoreUsers) != pdTRUE) {
                    // Improvável
                    printf("Erro critico ao encerrar o semaforo durante o reset!\n");
                    break;
                }
            }
            taskEXIT_CRITICAL(); // Sai da seção crítica

            uint32_t vagas_apos_reset = uxSemaphoreGetCount(xCountingSemaphoreUsers);
            printf("Sistema Resetado! Vagas: %lu / %u\n", vagas_apos_reset, MAX_USERS);
            strcpy(display_msg, "Sistema Resetado");

            // Atualiza o display (operação protegida por mutex)
            if (xSemaphoreTake(xMutexDisplay, portMAX_DELAY) == pdTRUE) {
                // Mostra 0 usuários ativos após o reset
                display_update(&ssd, 0, MAX_USERS, display_msg);
                xSemaphoreGive(xMutexDisplay);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_DELAY_MS)); // Pausa para polling do botão
    }
}

/**
 * @brief Tarefa responsável por fornecer feedback visual sobre a ocupação
 * do espaço através do LED RGB. Periodicamente, lê a contagem de usuários
 * (indiretamente, pelo número de vagas no semáforo) e atualiza a cor do LED
 * (Azul: vazio, Verde: com vagas, Amarelo: quase lotado, Vermelho: lotado).
 */
void vTaskFeedbackVisualLedRgb(void *pvParameters) {
    printf("Task Feedback LED RGB iniciada.\n");
    uint8_t usuarios_ativos;
    uint32_t vagas_atuais_disponiveis;

    while (true) {
        // Obtém o número atual de vagas disponíveis do semáforo de contagem
        vagas_atuais_disponiveis = uxSemaphoreGetCount(xCountingSemaphoreUsers);
        // Calcula o número de usuários ativos
        usuarios_ativos = MAX_USERS - vagas_atuais_disponiveis;

        // Atualiza a cor do LED RGB com base na ocupação
        rgb_led_set(usuarios_ativos, MAX_USERS);

        vTaskDelay(pdMS_TO_TICKS(RGB_UPDATE_DELAY_MS)); // Pausa antes da próxima atualização do LED
    }
}

/**
 * @brief Tarefa responsável por atualizar as informações no display OLED.
 * Periodicamente, lê a contagem de usuários (indiretamente, pelo número
 * de vagas no semáforo) e atualiza o display com o número de usuários ativos,
 * vagas disponíveis e uma mensagem de status padrão. O acesso ao display
 * é protegido por um mutex.
 */
void vTaskDisplayInfoOled(void *pvParameters) {
    ssd1306_t *disp = (ssd1306_t *)pvParameters; // Handle do display passado como parâmetro
    printf("Task Display Info OLED iniciada.\n");

    while (true) {
        uint32_t vagas_atuais_disponiveis;
        uint8_t usuarios_ativos;

        // Obtém o número atual de vagas e calcula usuários ativos
        vagas_atuais_disponiveis = uxSemaphoreGetCount(xCountingSemaphoreUsers);
        usuarios_ativos = MAX_USERS - vagas_atuais_disponiveis;

        // Pede o mutex para ter acesso exclusivo ao display
        if (xSemaphoreTake(xMutexDisplay, portMAX_DELAY) == pdTRUE) {
            // Atualiza o display. Passar NULL para a mensagem faz com que
            // a função display_update gere uma mensagem padrão baseada na contagem.
            display_update(disp, usuarios_ativos, MAX_USERS, NULL);

            xSemaphoreGive(xMutexDisplay); // Libera o mutex
        }
        vTaskDelay(pdMS_TO_TICKS(DISPLAY_UPDATE_DELAY_MS)); // Pausa antes da próxima atualização do display
    }
}

/**
 * @brief Tarefa responsável por controlar as animações na matriz de LEDs.
 * Periodicamente, lê a contagem de usuários (indiretamente, pelo semáforo de vagas)
 * e atualiza a matriz com um ícone animado (pulsante) que reflete o estado de
 * ocupação do espaço (livre, com vagas, quase cheio, lotado) ou uma animação
 * especial durante o reset do sistema.
 */
void vTaskLedMatrixControl(void *pvParameters) {
    printf("Task da matriz de leds iniciada.\n");
    uint8_t animation_step = 0; // Contador para controlar os frames da animação
    MatrixOccupationState_t determined_matrix_state = MATRIX_STATE_VAZIO; // Estado visual da matriz

    // Variáveis para controle da animação de reset na matriz
    bool matrix_currently_in_reset_animation = false;
    uint8_t matrix_reset_animation_step_count = 0;

    uint8_t prev_active_users_matrix = 0; // Para ajudar a detectar o evento de reset


    while (true) {
        uint32_t current_available_slots = uxSemaphoreGetCount(xCountingSemaphoreUsers);
        uint8_t current_active_users = MAX_USERS - current_available_slots;

        // Heurística para detectar um reset (contagem cai para 0 vindo de >0)
        // e disparar a animação de reset na matriz.
        // Uma flag global setada por vTaskResetSistema seria mais explícita.
        if (prev_active_users_matrix > 0 && current_active_users == 0 && !matrix_currently_in_reset_animation) {
            //printf("Matrix: Reset detectado, iniciando animacao de reset.\n");
            matrix_currently_in_reset_animation = true;
            matrix_reset_animation_step_count = 0;
        }
            // Caso contrário, determina o estado normal da matriz baseado na ocupação
            if (current_active_users == 0) {
                determined_matrix_state = MATRIX_STATE_VAZIO; 
            }
            else if (current_active_users >= MAX_USERS) {
                determined_matrix_state = MATRIX_STATE_CHEIO;
            }
            else if (current_active_users == MAX_USERS - 1) {
                determined_matrix_state = MATRIX_STATE_QUASE_CHEIO;
            }
            else {
                determined_matrix_state = MATRIX_STATE_VAGAS_LIVRES;
            }
            // Chama a função que atualiza a matriz com o estado e o passo da animação
            led_matrix_ocupacao(determined_matrix_state, animation_step);

        prev_active_users_matrix = current_active_users; // Guarda o estado atual para a próxima detecção de reset
        animation_step++; // Avança o passo da animação

        vTaskDelay(pdMS_TO_TICKS(MATRIX_DELAY_MS)); // Pausa antes da próxima atualização da matriz
    }
}