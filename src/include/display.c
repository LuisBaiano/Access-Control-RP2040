#include "display.h"
#include "config.h"
#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"


/**
  * @brief Inicializa a comunicação I2C e o display OLED SSD1306.
  *        Configura os pinos SDA e SCL, inicializa o periférico I2C e
  *        envia os comandos de configuração para o display.
  *
  * @param ssd Ponteiro para a estrutura de controle do display SSD1306 a ser inicializada.
  */
 void display_init(ssd1306_t *ssd) {
     // Inicializa I2C na porta e velocidade definidas
     i2c_init(I2C_PORT, 400 * 1000);
     // Configura os pinos GPIO para a função I2C
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
     // Habilita resistores de pull-up internos para os pinos I2C
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
     // Inicializa a estrutura do driver SSD1306 com os parâmetros do display
    ssd1306_init(ssd, WIDTH, HEIGHT, false, DISPLAY_ADDR, I2C_PORT);
     // Envia a sequência de comandos de configuração para o display
    ssd1306_config(ssd);
    ssd1306_fill(ssd, false);
    ssd1306_send_data(ssd);
    printf("Display inicializado.\n");
}

/**
  * @brief Exibe uma tela de inicialização no display OLED.
  *        Mostra um texto de título e o ícone do semáforo por alguns segundos.
  *
  * @param ssd Ponteiro para a estrutura de controle do display SSD1306.
  */
 void display_startup_screen(ssd1306_t *ssd) {
     // Limpa o display
    ssd1306_fill(ssd, false);
     // Calcula posições aproximadas para centralizar o texto
    uint8_t center_x_approx = ssd->width / 2;
    uint8_t start_y = 8;
     uint8_t line_height = 10; // Espaçamento vertical entre linhas

    ssd1306_rect(ssd, 0, 0, 127, 63, 1, false);
     // Define as strings a serem exibidas
     const char *line1 = "EMBARCATECH";
     const char *line2 = "PROJETO";
     const char *line3 = "PAINEL DE";
     const char *line4 = "CONTROLE RTOS";
    ssd1306_draw_string(ssd, line1, center_x_approx - (strlen(line1)*8)/2, start_y);
    ssd1306_draw_string(ssd, line2, center_x_approx - (strlen(line2)*8)/2, start_y + line_height);
    ssd1306_draw_string(ssd, line3, center_x_approx - (strlen(line3)*8)/2, start_y + 2*line_height);
    ssd1306_draw_string(ssd, line4, center_x_approx - (strlen(line4)*8)/2, start_y + 3*line_height);
    ssd1306_send_data(ssd);
    sleep_ms(2500);
    ssd1306_fill(ssd, false);
    ssd1306_send_data(ssd);
}

void display_update(ssd1306_t *ssd, uint8_t actual_num_users, uint8_t max_users, const char* frase) {
    if (!ssd) return;

    char contagem_str[25];   
    char vagas_str[20];       
    char status_str[32];  

    ssd1306_fill(ssd, false);

    ssd1306_rect(ssd, 0, 0, DISPLAY_WIDTH -1 , DISPLAY_HEIGHT -1 , true, false);

    const char* topo = "Ctrle de Acesso";
    uint8_t titulo = (DISPLAY_WIDTH / 2) - (strlen(topo) * 8 / 2);
    ssd1306_draw_string(ssd, topo, titulo, 3); 

    ssd1306_hline(ssd, 2, DISPLAY_WIDTH - 3, 13, true); 

    sprintf(contagem_str, "Ocupado: %u/%u", actual_num_users, max_users);
    ssd1306_draw_string(ssd, contagem_str, 5, 19); 

    uint8_t vagas = max_users > actual_num_users ? max_users - actual_num_users : 0;
    sprintf(vagas_str, "Vagas:   %u", vagas);
    ssd1306_draw_string(ssd, vagas_str, 5, 28); 

    ssd1306_hline(ssd, 2, DISPLAY_WIDTH - 3, 38, true); 

    if (frase && strlen(frase) > 0) {
        strncpy(status_str, frase, sizeof(status_str) - 1);
        status_str[sizeof(status_str) - 1] = '\0';
    } else {
        if (actual_num_users >= max_users) {
            strcpy(status_str, "Lotado!");
        } else if (vagas == 1 && actual_num_users == max_users - 1) {
            strcpy(status_str, "Ultima Vaga!");
        } else if (actual_num_users == 0) {
            strcpy(status_str, "Livre");
        } else {
            strcpy(status_str, "Entrada Ok");
        }
    }
    uint8_t msg_len = strlen(status_str);
    uint8_t msg_x = (DISPLAY_WIDTH / 2) - (msg_len * 8 / 2);
    if (msg_x < 2) msg_x = 2;
    ssd1306_draw_string(ssd, status_str, msg_x, 45);



    ssd1306_send_data(ssd);
}