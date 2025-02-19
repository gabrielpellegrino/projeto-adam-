#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2818b.pio.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
// Bibliotecas adicionadas por causa do Oled
#include <string.h>
#include <ctype.h>
#include "pico/binary_info.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"

const uint I2C_SDA = 14; // Pinos do Oled
const uint I2C_SCL = 15;

// Definição dos pinos
#define LED_RED 13
#define LED_GREEN 11
#define LED_BLUE 12
#define BUTTON_A 5
#define BUTTON_B 6
#define JOYSTICK_BUTTON 22
#define BUZZER 10 // Pino do buzzer
#define LED_PIN 7 // Pino para os LEDs WS2812B
#define MAX_SEQUENCE_LENGTH 10
#define LED_COUNT 25 // Número total de LEDs na matriz
#define NOTE_C4 261
#define NOTE_D4 0
#define NOTE_E4 329
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 466
#define NOTE_C5 523
#define NOTE_D5 587
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_G5 523  
#define NOTE_A5 880 

// Notas musicais para a música de comemoração (Ode à Alegria)
#define NOTE_E4  329
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  466
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_C4  261
#define NOTE_G3  196
#define NOTE_E3  165

// Pino e canal do microfone no ADC.
#define MIC_CHANNEL 2
#define MIC_PIN (26 + MIC_CHANNEL)

// Parâmetros e macros do ADC.
#define ADC_CLOCK_DIV 96.f
#define SAMPLES 200 // Número de amostras do ADC.
#define ADC_ADJUST(x) (x * 3.3f / (1 << 12u) - 1.65f) // Converte ADC para volts.
#define ADC_MAX 3.3f
#define ADC_STEP (3.3f / 5.f) // Intervalos de volume.
#define CLAP_THRESHOLD 0.6f  // Limiar reduzido para facilitar a detecção

// Código para o Oled
ssd1306_t ssd_bm; // Variável global para manter a instância do display

// Função para inicializar o display
void init_display() {
    // Inicialização do i2c
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    
    // Processo de inicialização completo do OLED SSD1306
    ssd1306_init(); 
}

// Música de comemoração (Ode à Alegria)
void play_celebration_music() {
    int melody[] = { 
        NOTE_E4, NOTE_E4, NOTE_F4, NOTE_G4, 
        NOTE_G4, NOTE_F4, NOTE_E4, NOTE_D5, 
        NOTE_C5, NOTE_C5, NOTE_D5, NOTE_E4 
    };

    int duration[] = { 
        200, 200, 200, 200, 
        200, 200, 300, 100, 
        200, 200, 200, 400 
    };

    for (int i = 0; i < 12; i++) {
        play_tone(melody[i], duration[i]);
        sleep_ms(50);
    }
}

void play_error_music() {
    int melody[] = { 
        NOTE_C4, NOTE_G3, NOTE_E3 
    };

    int duration[] = { 
        300, 300, 500 
    };

    for (int i = 0; i < 3; i++) {
        play_tone(melody[i], duration[i]);
        sleep_ms(50);
    }
}


// Função para exibir até 4 mensagens no display
void display_message(char *msg1, char *msg2, char *msg3, char *msg4) {
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length); // Limpa o display
    
    // Área de renderização
    struct render_area frame_area = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&frame_area);

    // Escreve as mensagens nas posições corretas (8 pixels de altura por linha)
    ssd1306_draw_string(ssd, 5, 0, msg1);
    ssd1306_draw_string(ssd, 5, 8, msg2);
    ssd1306_draw_string(ssd, 5, 16, msg3);
    ssd1306_draw_string(ssd, 5, 24, msg4);
    
    render_on_display(ssd, &frame_area);
}

// Estrutura para representar um pixel GRB (NeoPixel usa essa ordem).
typedef struct {
    uint8_t G, R, B;
} npLED_t;

// Buffer que armazena os valores dos LEDs.
npLED_t leds[LED_COUNT];

// Variáveis para controle da PIO.
PIO np_pio;
uint sm;

// Inicializa os LEDs NeoPixel
void npInit(uint pin) {
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;
    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0) {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true);
    }
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

    for (uint i = 0; i < LED_COUNT; ++i) {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}


// Define a cor de um LED com 30% do brilho original (redução de 70%)
void npSetLED(uint index, uint8_t r, uint8_t g, uint8_t b) {
    if (index < LED_COUNT) {
        leds[index].R = r * 0.3;  // Mantém apenas 30% do brilho original
        leds[index].G = g * 0.3;  // Mantém apenas 30% do brilho original
        leds[index].B = b * 0.3;  // Mantém apenas 30% do brilho original
    }
}

// Atualiza os LEDs da matriz
void npWrite() {
    for (uint i = 0; i < LED_COUNT; ++i) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
    sleep_us(100);
}

// Converte coordenadas (x, y) para índice no vetor de LEDs
int getIndex(int x, int y) {
    return (y % 2 == 0) ? (y * 5 + x) : (y * 5 + (4 - x));
}

// Matriz de LEDs para indicar acerto (brilho reduzido em 70%)
void acender_matriz() {
    uint32_t matriz_hex[5][5] = {
        {0x000000, 0x000000, 0xFFFFFF, 0x000000, 0x000000},
        {0x000000, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0x000000},
        {0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF},
        {0x000000, 0x000000, 0xFFFFFF, 0x000000, 0x000000},
        {0x000000, 0x000000, 0x000000, 0x000000, 0x000000}
    };

    for (int linha = 0; linha < 5; linha++) {
        for (int coluna = 0; coluna < 5; coluna++) {
            int posicao = getIndex(coluna, linha);
            uint32_t color = matriz_hex[linha][coluna];
            npSetLED(posicao, (color >> 16 & 0xFF) * 0.3, (color >> 8 & 0xFF) * 0.3, (color & 0xFF) * 0.3);
        }
    }
    npWrite();
}

// Matriz de LEDs para indicar erro (brilho reduzido em 70%)
void acender_matriz_erro() {
    uint32_t matriz_hex_erro[5][5] = {
        {0x000000, 0x000000, 0xFF0000, 0x000000, 0x000000},
        {0x000000, 0xFF0000, 0xFF0000, 0xFF0000, 0x000000},
        {0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000},
        {0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000},
        {0x000000, 0xFF0000, 0x000000, 0xFF0000, 0x000000}
    };

    for (int linha = 0; linha < 5; linha++) {
        for (int coluna = 0; coluna < 5; coluna++) {
            int posicao = getIndex(coluna, linha);
            uint32_t color = matriz_hex_erro[linha][coluna];
            npSetLED(posicao, (color >> 16 & 0xFF) * 0.3, (color >> 8 & 0xFF) * 0.3, (color & 0xFF) * 0.3);
        }
    }

    play_error_music(); // Toca a música de erro ao errar
    npWrite();
}


// Apaga todos os LEDs
void apagar_matriz() {
    for (uint i = 0; i < LED_COUNT; ++i) {
        npSetLED(i, 0, 0, 0);
    }
    npWrite();
}

// Toca um tom no buzzer
void play_tone(uint freq, uint duration_ms) {
    if (freq > 0) {
        uint delay = 1000000 / freq;
        uint cycles = (freq * duration_ms) / 1000;
        for (uint i = 0; i < cycles; i++) {
            gpio_put(BUZZER, 1);
            sleep_us(delay / 2);
            gpio_put(BUZZER, 0);
            sleep_us(delay / 2);
        }
    } else {
        gpio_put(BUZZER, 0);
        sleep_ms(duration_ms);
    }
}

// Música inicial (Super Mario Theme - Ajustada para velocidade normal)
// Música inicial (Super Mario Theme) com LEDs reduzidos
// Música inicial (Super Mario Theme) com LEDs reduzidos em 70%
void play_music() {
    int melody[] = { 
        NOTE_E5, NOTE_E5, 0, NOTE_E5, 0, NOTE_C5, NOTE_E5, 0, 
        NOTE_G5, 0, NOTE_G4, 0, NOTE_C5, 0, NOTE_G4, 0, 
        NOTE_E4, NOTE_A4, 0, NOTE_B4, NOTE_A4, NOTE_A4, NOTE_G4, NOTE_E5, 
        NOTE_G5, NOTE_A5, NOTE_F5, NOTE_G5, NOTE_E5, NOTE_C5, NOTE_D5, NOTE_B4,

        NOTE_G5, NOTE_E5, NOTE_G5, NOTE_A5, 0, NOTE_F5, NOTE_G5, 0, 
        NOTE_E5, NOTE_C5, NOTE_D5, NOTE_B4, NOTE_C5, NOTE_A4, NOTE_G4, 0,
        NOTE_E5, NOTE_E5, 0, NOTE_E5, 0, NOTE_C5, NOTE_E5, 0, 
        NOTE_G5, 0, NOTE_G4, 0, NOTE_C5, 0, NOTE_G4, 0 
    };

    int duration[] = { 
        150, 150, 100, 150, 100, 150, 150, 100, 
        300, 100, 300, 100, 300, 100, 300, 100, 
        150, 150, 100, 150, 150, 150, 150, 100, 
        150, 150, 150, 150, 150, 150, 150, 400,

        150, 150, 150, 200, 100, 150, 150, 100, 
        150, 150, 150, 200, 150, 150, 150, 100, 
        150, 150, 100, 150, 100, 150, 150, 100, 
        300, 100, 300, 100, 300, 100, 300, 100 
    };

    int colors[] = { 
        0, 1, 2, 0, 1, 2, 0, 1, 
        2, 0, 1, 2, 0, 1, 2, 0, 
        1, 2, 0, 1, 2, 0, 1, 2, 
        0, 1, 2, 0, 1, 2, 0, 1,

        2, 0, 1, 2, 0, 1, 2, 0, 
        1, 2, 0, 1, 2, 0, 1, 2, 
        0, 1, 2, 0, 1, 2, 0, 1, 
        2, 0, 1, 2, 0, 1, 2, 0
    };

    for (int i = 0; i < 64; i++) {
        // Acende a matriz de LEDs na cor correspondente com brilho reduzido (70%)
        for (int j = 0; j < LED_COUNT; j++) {
            switch (colors[i]) {
                case 0: // Vermelho reduzido
                    npSetLED(j, 255 * 0.3, 0, 0);
                    break;
                case 1: // Verde reduzido
                    npSetLED(j, 0, 255 * 0.3, 0);
                    break;
                case 2: // Azul reduzido
                    npSetLED(j, 0, 0, 255 * 0.3);
                    break;
            }
        }

        npWrite();
        play_tone(melody[i], duration[i]);

        for (int j = 0; j < LED_COUNT; j++) {
            npSetLED(j, 0, 0, 0);
        }
        npWrite();
        sleep_ms(50);
    }
}


// Inicializa o hardware
void hardware_init() {
    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);
    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    gpio_init(LED_BLUE);
    gpio_set_dir(LED_BLUE, GPIO_OUT);
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
    gpio_init(JOYSTICK_BUTTON);
    gpio_set_dir(JOYSTICK_BUTTON, GPIO_IN);
    gpio_pull_up(JOYSTICK_BUTTON);
    gpio_init(BUZZER);
    gpio_set_dir(BUZZER, GPIO_OUT);
}


// Função principal da fase
bool play_phase(int sequence_length) {
    int sequence[MAX_SEQUENCE_LENGTH];

    //  Gerar sequência aleatória
    for (int i = 0; i < sequence_length; i++) {
        sequence[i] = rand() % 3;
    }

    //  Exibir mensagem no OLED sobre a nova fase
    char phase_msg[20];
    sprintf(phase_msg, "Fase %d iniciando!", sequence_length);
    display_message(phase_msg, "Memorize a", "sequência...", "");
    sleep_ms(2000);

    //  Exibir a sequência para o jogador no OLED
    for (int i = 0; i < sequence_length; i++) {
        char step_msg[20];
        sprintf(step_msg, "Passo %d/%d", i + 1, sequence_length);
        
        // Define cor da sequência e exibe no OLED
        if (sequence[i] == 0) {
            display_message(step_msg, "Cor: Vermelho", "", "");
            gpio_put(LED_RED, 1);
        } else if (sequence[i] == 1) {
            display_message(step_msg, "Cor: Verde", "", "");
            gpio_put(LED_GREEN, 1);
        } else {
            display_message(step_msg, "Cor: Azul", "", "");
            gpio_put(LED_BLUE, 1);
        }

        sleep_ms(500);
        gpio_put(LED_RED, 0);
        gpio_put(LED_GREEN, 0);
        gpio_put(LED_BLUE, 0);
        sleep_ms(500);
    }

    //  Exibir mensagem no OLED pedindo para o jogador repetir a sequência
    display_message("Agora sua vez!", "Reproduza a", "sequencia!", "");
    
    //  Loop para verificar se o jogador repete a sequência corretamente
    for (int i = 0; i < sequence_length; i++) {
        bool correct_input = false;
        uint64_t start_time = to_ms_since_boot(get_absolute_time());

        while (!correct_input) {
            //  Verificar se o tempo acabou
            if (to_ms_since_boot(get_absolute_time()) - start_time > 5000) {
                display_message( "","Tempo esgotado!", "Tente novamente.", "");
                acender_matriz_erro();  // Acende LEDs de erro
                sleep_ms(2000);
                apagar_matriz();        // Apaga LEDs
                return false;
            }

            // 🔹 Verificar se o jogador pressionou um botão correto
            if (!gpio_get(BUTTON_A)) {
                display_message("Botao", "pressionado:", "Vermelho", "");
                play_tone(1000, 100);
                gpio_put(LED_RED, 1);
                sleep_ms(200);
                gpio_put(LED_RED, 0);
                if (sequence[i] == 0) {
                    correct_input = true;
                } else {
                    display_message("","Erro!", "Pressionou", "errado!");
                    acender_matriz_erro();
                    sleep_ms(2000);
                    apagar_matriz();
                    return false;
                }
            }

            if (!gpio_get(BUTTON_B)) {
                display_message("", "Botao", "pressionado:", "Verde");
                play_tone(1200, 100);
                gpio_put(LED_GREEN, 1);
                sleep_ms(200);
                gpio_put(LED_GREEN, 0);
                if (sequence[i] == 1) {
                    correct_input = true;
                } else {
                    display_message("", "Erro!", "Pressionou", "errado!");
                    acender_matriz_erro();
                    sleep_ms(2000);
                    apagar_matriz();
                    return false;
                }
            }

            if (!gpio_get(JOYSTICK_BUTTON)) {
                display_message("", "Botao","pressionado:", "Azul");
                play_tone(800, 100);
                gpio_put(LED_BLUE, 1);
                sleep_ms(200);
                gpio_put(LED_BLUE, 0);
                if (sequence[i] == 2) {
                    correct_input = true;
                } else {
                    display_message("============", "Erro!", "Pressionou", "errado!");
                    acender_matriz_erro();
                    sleep_ms(2000);
                    apagar_matriz();
                    return false;
                }
            }
        }
    }

    //  Exibir mensagem de sucesso no OLED
    display_message("Fase concluida!", "Prepare-se", "para a", "proxima!");
    acender_matriz();  // LEDs piscam para mostrar sucesso
    play_celebration_music();  // Música de comemoração
    sleep_ms(2000);
    apagar_matriz();
    
    return true;
}

void display_phase_result(bool success, int phase) {
    if (success) {
        char msg[20];
        sprintf(msg, "Fase %d concluida!", phase);
        display_message("Parabens!", msg, "Prepare-se", "para a" "proxima!");
    } else {
        display_message("Voce errou!", "Tente novamente!", "", "");
    }
    sleep_ms(2000);  // Exibe a mensagem por 2 segundos
}


// Inicializa o ADC do microfone
void init_adc() {
    adc_init();
    adc_gpio_init(MIC_PIN);
    adc_select_input(MIC_CHANNEL);
}


// Atualiza a matriz de LEDs para mostrar uma animação progressiva com cores diferentes
void update_clap_leds(int clap_count) {
    // Matrizes para cada estado da animação
    uint32_t matriz_1[5][5] = { // Primeira palma (LEDs azuis)
        {0x000000, 0x000000, 0x0000FF, 0x000000, 0x000000},
        {0x000000, 0x000000, 0x0000FF, 0x000000, 0x000000},
        {0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF},
        {0x000000, 0x000000, 0x0000FF, 0x000000, 0x000000},
        {0x000000, 0x000000, 0x0000FF, 0x000000, 0x000000}
    };

    uint32_t matriz_2[5][5] = { // Segunda palma (LEDs verdes)
        {0x000000, 0x00FF00, 0x00FF00, 0x00FF00, 0x000000},
        {0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00},
        {0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00},
        {0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00},
        {0x000000, 0x00FF00, 0x00FF00, 0x00FF00, 0x000000}
    };

    uint32_t matriz_3[5][5] = { // Terceira palma (LEDs roxos, com mais apagados)
        {0x000000, 0x000000, 0x800080, 0x000000, 0x000000},
        {0x000000, 0x800080, 0x800080, 0x800080, 0x000000},
        {0x800080, 0x800080, 0x800080, 0x800080, 0x800080},
        {0x000000, 0x800080, 0x000000, 0x800080, 0x000000},
        {0x000000, 0x000000, 0x000000, 0x000000, 0x000000}
    };

    // Escolher qual matriz acender
    uint32_t (*selected_matriz)[5][5];
    if (clap_count == 1) selected_matriz = &matriz_1;
    else if (clap_count == 2) selected_matriz = &matriz_2;
    else selected_matriz = &matriz_3;

    // Acender a matriz correspondente
    for (int linha = 0; linha < 5; linha++) {
        for (int coluna = 0; coluna < 5; coluna++) {
            int posicao = getIndex(coluna, linha);
            uint32_t color = (*selected_matriz)[linha][coluna];
            npSetLED(posicao, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
        }
    }

    npWrite(); // Atualiza a matriz de LEDs
}

// Função para capturar palmas e iniciar o jogo
bool detect_claps(const char* message) {
    int clap_count = 0;
    uint64_t last_clap_time = 0;

    display_message(message, "Bata 3 palmas!", "", "");

    while (clap_count < 3) {
        uint16_t sample = adc_read();
        float voltage = ADC_ADJUST(sample);

        // Se a tensão for maior que o limiar, considera como palma
        if (voltage > CLAP_THRESHOLD) {
            uint64_t now = to_ms_since_boot(get_absolute_time());

            // Garantir tempo mínimo entre palmas (300 ms)
            if (now - last_clap_time > 300) {
                clap_count++;
                last_clap_time = now;
                printf("Palma detectada! Total: %d\n", clap_count);
                play_tone(1000, 100); // Feedback sonoro rápido

                // Atualizar a animação na matriz de LEDs
                update_clap_leds(clap_count);

                // Atualizar mensagem no OLED
                char msg[20];
                sprintf(msg, "Palmas: %d/3", clap_count);
                display_message(message, msg, "", "");
            }
        }
        sleep_ms(10); // Pequena espera para evitar leituras erradas
    }

    display_message("","Muito bem!", "Iniciando", "o jogo...");
    sleep_ms(1000);  // Pequena pausa antes de iniciar a música
    apagar_matriz(); // Agora apagamos a matriz antes do jogo
    return true;
}

// Última fase das 3 palmas
bool play_clap_phase() {
    return detect_claps("Ultima fase!");
}

int main() {
    stdio_init_all();       // Inicializa a comunicação serial (para debug)
    init_display();         // Inicializa o display OLED
    hardware_init();        // Inicializa os pinos e periféricos
    npInit(LED_PIN);        // Inicializa os LEDs
    init_adc();             // Inicializa o microfone

    // Esperar 3 palmas para iniciar
    detect_claps("Bem-vindo!");

    // Agora toca a música do Mario direto após as 3 palmas
    display_message("===========","Projeto Adam.", "Oferecimento", "EMBARCATECH...");
    play_music();
    
    int phase = 1, sequence_length = 3;
    while (phase <= 4) {
        if (play_phase(sequence_length)) phase++, sequence_length++;
    }

    // Última fase das palmas
    if (play_clap_phase()) {
        display_message("Parabens!", "Jogo concluido!", "", "");
    }

    return 0;
}
