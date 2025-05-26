
# Painel de Controle Interativo com Acesso Concorrente usando FreeRTOS na BitDogLab

## Índice

- [Painel de Controle Interativo com Acesso Concorrente usando FreeRTOS na BitDogLab](#painel-de-controle-interativo-com-acesso-concorrente-usando-freertos-na-bitdoglab)
  - [Índice](#índice-1)
  - [Objetivos](#objetivos-1)
  - [Descrição do Projeto](#descrição-do-projeto-1)
    - [Mecanismos de Sincronização](#mecanismos-de-sincronização)
    - [Regras de Funcionamento e Feedback](#regras-de-funcionamento-e-feedback)
    - [Arquitetura FreeRTOS](#arquitetura-freertos-1)
  - [Funcionalidades Implementadas](#funcionalidades-implementadas-1)
  - [Requisitos Técnicos Atendidos](#requisitos-técnicos-atendidos-1)
  - [Como Compilar e Executar](#como-compilar-e-executar-1)
    - [Requisitos de Hardware](#requisitos-de-hardware-1)
    - [Requisitos de Software](#requisitos-de-software-1)
    - [Passos para Compilação e Execução](#passos-para-compilação-e-execução-1)
  - [Estrutura do Código](#estrutura-do-código-1)
    - [Principais Arquivos](#principais-arquivos-1)
    - [Sincronização entre Tarefas](#sincronização-entre-tarefas)
  - [Sugestões para Criatividade e Impacto Social](#sugestões-para-criatividade-e-impacto-social-1)
  - [Demonstrativo em Vídeo](#demonstrativo-em-vídeo-1)

## Objetivos

* Aplicar e consolidar os conhecimentos sobre mecanismos de sincronização do FreeRTOS: **semáforos de contagem, semáforos binários (com interrupção) e mutexes.**
* Desenvolver um sistema embarcado multitarefa funcional na placa BitDogLab com o microcontrolador RP2040.
* Implementar o controle de acesso a um recurso compartilhado (vagas em um espaço) usando um semáforo de contagem.
* Garantir o acesso seguro e exclusivo a um periférico compartilhado (Display OLED) utilizando um mutex.
* Utilizar interrupção de hardware para um evento crítico (reset do sistema) e sinalizar uma tarefa via semáforo binário (adaptado para flag setada por ISR e polling na tarefa, conforme desenvolvimento).
* Controlar múltiplos periféricos: Botões (GPIO e Interrupção), LED RGB (GPIO), Matriz de LEDs WS2812 (PIO), Buzzer (PWM) e Display OLED SSD1306 (I2C).
* Criar um sistema com feedback visual e sonoro claro para as diferentes interações e estados do sistema.
* Estruturar o código de forma modular e comentada.

## Descrição do Projeto

Este projeto implementa um "Painel de Controle Interativo" na placa BitDogLab (RP2040) utilizando o FreeRTOS para simular o gerenciamento de acesso de usuários a um espaço com capacidade limitada (ex: laboratório, biblioteca). O foco principal é a correta aplicação dos mecanismos de sincronização do FreeRTOS para coordenar tarefas concorrentes.

### Mecanismos de Sincronização

* **Semáforo de Contagem (`xCountingSemaphoreUsers`):** Gerencia o número de vagas disponíveis no espaço. É inicializado com `MAX_USERS` (capacidade máxima) e a contagem representa o número de vagas livres.
* **Mutex (`xMutexDisplay`):** Protege o acesso ao display OLED, garantindo que apenas uma tarefa possa modificá-lo por vez, evitando corrupção visual.
* **Sinalização de Reset (Botão Joystick):** Uma interrupção de hardware no botão do joystick seta uma flag. Uma tarefa dedicada (`vTaskResetSistema`) faz polling desta flag (via `buttons_joystick_pressed()`) para iniciar o processo de reset. (Esta é uma adaptação do requisito original de usar um semáforo binário dado diretamente pela ISR, integrando com a biblioteca `buttons.c` existente).

### Regras de Funcionamento e Feedback

O sistema responde a três ações principais:

1. **Entrada de Usuário (Botão A):**
   * A `vTaskEntradaUsuarios` tenta decrementar o contador de vagas disponíveis (tomar do `xCountingSemaphoreUsers`).
   * **Sucesso:** Atualiza o display com "Entrada OK" e a nova contagem.
   * **Falha (Lotado):** Emite um beep curto no buzzer e exibe "Lotado!" no display.
2. **Saída de Usuário (Botão B):**
   * A `vTaskSaidaUsuarios` verifica se há usuários e incrementa o contador de vagas disponíveis (dando ao `xCountingSemaphoreUsers`).
   * Atualiza o display com "Saida OK" ou "Vazio".
3. **Reset do Sistema (Botão Joystick):**
   * A `vTaskResetSistema`, ao detectar o acionamento, emite um beep duplo.
   * Restaura o `xCountingSemaphoreUsers` para a capacidade máxima (todas as vagas disponíveis).
   * Atualiza o display para "Sistema Resetado" e 0 usuários ativos.

O feedback visual inclui:

* **LED RGB:**
  * Azul: Nenhum usuário.
  * Verde: Usuários presentes (1 a `MAX_USERS - 2`).
  * Amarelo: Apenas 1 vaga restante (`MAX_USERS - 1` usuários).
  * Vermelho: Capacidade máxima atingida (`MAX_USERS` usuários).
* **Matriz de LEDs:** Exibe ícones animados e pulsantes correspondentes aos estados acima (círculo azul, checkmark verde, exclamação amarela piscante, 'X' vermelho pulsante) e um flash branco durante o reset.
* **Display OLED:** Mostra o título, contagem de "Ocupado", "Vagas" e mensagens de status.

### Arquitetura FreeRTOS

O sistema é gerenciado pelo FreeRTOS e é composto por múltiplas tarefas concorrentes:

* `vTaskEntradaUsuarios`: Gerencia a lógica de entrada.
* `vTaskSaidaUsuarios`: Gerencia a lógica de saída.
* `vTaskResetSistema`: Gerencia a lógica de reset.
* `vTaskFeedbackVisualLedRgb`: Controla o LED RGB baseado na ocupação.
* `vTaskDisplayInfoOled`: Atualiza periodicamente o display OLED.
* `vTaskLedMatrixControl`: Controla as animações na matriz de LEDs.

A sincronização é crucial: o semáforo de contagem para as vagas e o mutex para o display.

## Funcionalidades Implementadas

```
✅ Controle de entrada de usuários via Botão A.
✅ Controle de saída de usuários via Botão B.
✅ Reset do sistema e da contagem de usuários via Botão do Joystick.
✅ Gerenciamento da capacidade máxima (MAX_USERS) usando semáforo de contagem (`xCountingSemaphoreUsers`).
✅ Proteção de acesso concorrente ao display OLED utilizando mutex (`xMutexDisplay`).
✅ Detecção do botão de reset via interrupção de hardware (setando flag interna no módulo buttons.c).
✅ Feedback visual no LED RGB indicando 4 níveis de ocupação (Vazio, Vagas, Última Vaga, Lotado).
✅ Exibição de informações no display OLED:
    - Título "Controle Acesso".
    - Contagem de "Ocupado: X/Y".
    - Contagem de "Vagas: Z".
    - Mensagens de status dinâmicas (ex: "Entrada OK", "Lotado!", "Sistema Resetado").
    - Layout organizado com borda e linhas divisórias.
✅ Sinalização sonora no buzzer:
    - Beep curto ao tentar entrar com o sistema cheio.
    - Beep duplo ao resetar o sistema.
✅ Feedback visual animado na Matriz de LEDs 5x5 WS2812:
    - Livre: Círculo azul pulsante.
    - Ocupado com Vagas: Checkmark verde pulsante.
    - Quase Cheio: Exclamação amarela piscante.
    - Lotado: 'X' vermelho com efeito de "respiração".
    - Reset: Flash branco na matriz.
✅ Utilização de múltiplas tarefas FreeRTOS dedicadas.
✅ Código estruturado em múltiplos arquivos (`main.c`, `hardware_config.h`, e módulos de driver).
✅ Comentários no código, incluindo formato Javadoc para as tarefas principais.
```

## Requisitos Técnicos Atendidos

1. **Uso de FreeRTOS com pelo menos 3 tarefas:** Implementado com 6 tarefas principais.
2. **Uso correto de `xSemaphoreCreateCounting()`:** Utilizado para `xCountingSemaphoreUsers` para gerenciar as vagas.
3. **Uso correto de `xSemaphoreCreateBinary()` (Adaptado):** A lógica de reset usa uma flag setada por ISR e polling pela tarefa, o que cumpre o requisito de "interrupção para o botão de reset", mas não usa explicitamente `xSemaphoreCreateBinary()` para esta sinalização direta ISR->Tarefa. *(Se uma implementação estrita com semáforo binário dado pela ISR fosse exigida, o módulo `buttons.c` precisaria ser modificado para dar `xSemaphoreGiveFromISR` e `vTaskResetSistema` faria `xSemaphoreTake` nele)*.
4. **Uso correto de `xSemaphoreCreateMutex()`:** Utilizado para `xMutexDisplay` para proteger o acesso ao OLED.
5. **Utilizar interrupção para o botão de reset (joystick):** O `buttons.c` configura a interrupção que seta uma flag consumida pela `vTaskResetSistema`.
6. **Garantir que o acesso ao display seja protegido com mutex:** Cumprido.

## Como Compilar e Executar

### Requisitos de Hardware

* Placa de desenvolvimento **BitDogLab** (com RP2040).
* Cabo Micro-USB.

### Requisitos de Software

* **VS Code** com a extensão Pico-W-Go ou configuração manual do toolchain ARM e Pico SDK.
* **Pico SDK** (v1.5.1 ou mais recente).
* **FreeRTOS Kernel** (como submódulo ou em caminho configurado no `CMakeLists.txt`).
* **Git** (opcional).
* Terminal Serial (ex: Monitor Serial do VS Code, Baudrate 115200).

### Passos para Compilação e Execução

1. **Obter o Código:** Clone o repositório ou organize os arquivos do projeto.
2. **Submódulos:** Se o FreeRTOS for um submódulo, execute `git submodule update --init --recursive`.
3. **Configurar `CMakeLists.txt`:** Verifique os caminhos para o SDK e FreeRTOS, e se `PICO_BOARD` está correto.
4. **Compilar:**
   * VS Code: Função de build (Ctrl+Shift+B).
   * Linha de Comando: `mkdir build && cd build && cmake .. && make`
5. **Carregar Firmware:** Pressione BOOTSEL, conecte a placa, copie o `.uf2` da pasta `build` para o drive `RPI-RP2`.
6. **Testar:**
   * Observe a tela de inicialização no OLED.
   * Pressione Botão A para simular entradas e Botão B para saídas.
   * Observe as mudanças no display, LED RGB e Matriz de LEDs.
   * Tente exceder a capacidade para ouvir o beep de "lotado".
   * Pressione o botão do joystick para resetar o sistema e observe o beep duplo e a reinicialização da contagem e dos visuais.

## Estrutura do Código

O código está organizado nas seguintes pastas e arquivos principais:

### Principais Arquivos

* `main.c`: Inicialização do sistema, criação dos semáforos/mutex, criação das tarefas FreeRTOS e implementações das tarefas.
* `include/hardware_management/hardware_config.h`: Definições de pinos, constantes do sistema, parâmetros do FreeRTOS e declarações `extern` dos handles de semáforos/mutex.
* `src/hardware_management/buttons.c` e `include/hardware_management/buttons.h`: Lógica para inicialização e leitura dos botões (A, B, Joystick), incluindo debounce e tratamento de interrupção para o joystick.
* `src/hardware_management/buzzer.c` e `include/hardware_management/buzzer.h`: Funções para controle do buzzer via PWM.
* `src/hardware_management/rgb_led.c` e `include/hardware_management/rgb_led.h`: Funções para controle do LED RGB.
* `src/hardware_management/display.c` e `include/hardware_management/display.h`: Funções para inicialização do display OLED e atualização do painel de informações.
* `src/hardware_management/led_matrix.c` e `include/hardware_management/led_matrix.h`: Lógica para controle da matriz de LEDs WS2812 via PIO, incluindo as animações.
* `pio/led_matrix.pio`: Código em assembly PIO para a matriz de LEDs.
* `lib/ssd1306/`: Biblioteca externa para o controlador do display OLED.
* `FreeRTOSConfig.h`: Configurações do kernel FreeRTOS.

### Sincronização entre Tarefas

* **`xCountingSemaphoreUsers`:** Controla o acesso às "vagas". Usado por `vTaskEntradaUsuarios`, `vTaskSaidaUsuarios`, `vTaskResetSistema`. Lido por `vTaskFeedbackVisualLedRgb`, `vTaskDisplayInfoOled`, `vTaskLedMatrixControl`.
* **`xMutexDisplay`:** Garante acesso serializado ao display OLED. Usado por `vTaskEntradaUsuarios`, `vTaskSaidaUsuarios`, `vTaskResetSistema`, `vTaskDisplayInfoOled` quando estas precisam escrever no display.
* **Botão de Reset:** A interrupção do joystick (em `buttons.c`) ativa uma flag volátil. A `vTaskResetSistema` verifica esta flag periodicamente (polling).

## Criatividade e Impacto Social (Conforme Sugestões Anteriores)

O projeto visa ser um sistema de controle de acesso claro e informativo:

* **Feedback Multimodal:** A combinação de Display OLED, LED RGB, Matriz de LEDs animada e Buzzer oferece redundância e clareza no feedback ao usuário sobre o estado do sistema.
* **Animações na Matriz:** Os ícones pulsantes e piscantes na matriz de LEDs tornam o feedback mais dinâmico e atraente do que simples luzes estáticas.
* **Acessibilidade:** Os beeps distintos para "lotado" e "reset" fornecem um canal de informação não visual, o que é um passo em direção à acessibilidade para pessoas com deficiência visual.
* **Relevância:** Simula um cenário prático e comum de gerenciamento de lotação, aplicável a diversos ambientes como salas de estudo, laboratórios pequenos, ou áreas de evento com controle de capacidade.

## Demonstrativo em Vídeo

https://drive.google.com/drive/folders/17Tob_1EjhvcNJ2YvyBXKxk2gQBSUH7Mt?usp=sharing
