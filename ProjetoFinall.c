#include "pico/stdlib.h"
#include "ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/watchdog.h"  
#include "ws2812.pio.h"
#include "music.h"
#include "pico/time.h"
#include "pico/stdio.h"
#include <stdio.h>
#include <string.h>

#define BUTTON_A 5
#define BUTTON_B 6
#define BUZZER_A 21
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define SSD1306_ADDR 0x3C
#define DEBOUNCE_TIME 200  // Definindo corretamente o debounce

#define WS2812_PIN 7
#define NLEDS 25

static PIO pio;
static int sm;
static uint dma_chan;
static uint32_t fitaEd[NLEDS];

ssd1306_t ssd;
volatile uint32_t last_press_time_A = 0;
volatile uint32_t last_press_time_B = 0;

const char *games[] = {"Pokemon", "Sonic", "Pacman", "Zelda", "Undertale"};
const int num_games = 5;
int current_game = 0;

bool button_A_pressed = false;  
bool button_B_pressed = false;  

// Declaração de funções
void button_irq_handler(uint gpio, uint32_t events);
void setup_gpio();
void setup_i2c();
void setup_adc();
void setup_ss1306();
void ws2812_init();
void update_display();
void pwm_init_buzzer(uint pin);
void joystick_loop();
void check_buttons();
void atualizaFita();
void draw_pokeball();
void draw_sonic();
void draw_pacman();
void draw_link();
void draw_sans();

int main() {
    stdio_init_all();

    // Inicializações
    setup_gpio();
    setup_i2c();
    setup_adc();
    setup_ss1306();
    ws2812_init();
    pwm_init_buzzer(BUZZER_A);
    update_display();

    memset(fitaEd, 0, sizeof(fitaEd));
    atualizaFita();

    while (true) {
        if(!button_A_pressed){
            joystick_loop();
        }
        if(button_A_pressed){
            button_A_pressed = false;
            switch (current_game) {
                case 0:
                    draw_pokeball();
                    play_Pokemon(BUZZER_A);
                    break;
                case 1:
                    draw_sonic();
                    play_Sonic(BUZZER_A);
                    break;
                case 2:
                    draw_pacman();
                    play_Pacman(BUZZER_A);
                    break;
                case 3:
                    draw_link();
                    play_Zelda(BUZZER_A);
                    break;
                case 4:
                    draw_sans();
                    play_Megalovania(BUZZER_A);
                    break;
                }
        }
        sleep_ms(200);
    }   

    return 0;
}

// **Configura os GPIOs**
void setup_gpio() {
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &button_irq_handler);

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &button_irq_handler);
}

// **Inicializa o ADC do joystick**
void setup_adc() {
    adc_init();
    adc_gpio_init(26);
    adc_gpio_init(27);
}

// **Inicializa o PWM para o buzzer**
void pwm_init_buzzer(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_set_enabled(slice_num, true);
}

// **Inicializa o I2C para o display OLED**
void setup_i2c() {
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

// **Inicializa o display OLED SSD1306**
void setup_ss1306() {
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, SSD1306_ADDR, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

// **Inicializa o WS2812 no PIO**
void ws2812_init() {
    pio = pio0;
    sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, false);
    dma_chan = dma_claim_unused_channel(true);
}

// **Rotina de interrupção do botão A**
void button_irq_handler(uint gpio, uint32_t events) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if(gpio == BUTTON_A){
        if (current_time - last_press_time_A > DEBOUNCE_TIME) {
            last_press_time_A = current_time;
            button_A_pressed = true;
        }
    }
    if(gpio == BUTTON_B){
        if (current_time - last_press_time_B > DEBOUNCE_TIME) {
            last_press_time_B = current_time;
            button_B_pressed = true;   
            watchdog_reboot(0, 0, 0);
        }
    }
}

// **Loop do joystick**
void joystick_loop() {
    uint16_t joystick_x_position;
    while (true) {
        adc_select_input(1);
        joystick_x_position = adc_read();

        if (joystick_x_position < 1000) {
            if (current_game > 0) {
                current_game--;
                update_display();
            }
        } else if (joystick_x_position > 3000) {
            if (current_game < num_games - 1) {
                current_game++;
                update_display();
            }
        }
        if(button_A_pressed){
            break;
        }

        sleep_ms(200);
    }
}

// **Atualiza o display**
void update_display() {
    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, games[current_game], 40, 20);
    ssd1306_send_data(&ssd);
}




void atualizaFita() {
    dma_channel_wait_for_finish_blocking(dma_chan);
    while (!pio_sm_is_tx_fifo_empty(pio, sm)) {
        sleep_us(10);
    }
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, true));
    dma_channel_configure(dma_chan, &c, &pio->txf[sm], fitaEd, NLEDS, true);
    sleep_us(300);
}

// **Função para converter cor RGB para formato WS2812**
inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(g) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(b) << 8);
}


// **Função para desenhar a Pokébola na matriz de LED 5x5**
void draw_pokeball() {
    // Definição de cores
    uint32_t R = urgb_u32(3, 0, 0);    // Vermelho
    uint32_t W = urgb_u32(3, 3, 3); // Branco

    // Matriz 5x5 da Pokébola
    uint32_t pokeball[5][5] = {
        {0,  W,  W,  W,  0},
        {W,  W,  W,  W,  W},
        {W,  W,  0,  W,  W},
        {R,  R,  R,  R,  R},
        {0,  R,  R,  R,  0}
    };

    // Enviar os pixels para os LEDs WS2812
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            int index = y * 5 + x;
            fitaEd[index] = pokeball[y][x]; // Atualiza os LEDs na fita
        }
    }

    atualizaFita();  // Envia a matriz corrigida para os LEDs
}

// **Função para desenhar o Sonic na Matriz de LED 5x5**
void draw_sonic() {
    // **Definição de Cores**
    uint32_t B = urgb_u32(0, 0, 10);   
    uint32_t W = urgb_u32(10, 10, 10);
    uint32_t R = urgb_u32(10, 0, 0);   
    uint32_t X = urgb_u32(0, 0, 0);
    uint32_t P = urgb_u32(10, 0, 10);      

    // **Matriz 5x5 do Sonic**
    uint32_t sonic[25] = {
        X,  R,  R,  R,  R,  
        X,  B,  B,  B,  X,  
        B,  B,  B,  B,  B,  
        W,  W,  B,  B,  X,  
        B,  B,  B,  B,  B
    };

    // **Enviar a matriz para os LEDs**
    for (int i = 0; i < NLEDS; i++) {
        fitaEd[i] = sonic[i];
    }

    atualizaFita();  // Atualiza os LEDs para exibir o Sonic
}


void draw_sans() {
    // **Definição de Cores**
    uint32_t B = urgb_u32(0, 0, 3);   
    uint32_t W = urgb_u32(3, 3, 3);
    uint32_t C = urgb_u32(0, 70, 70); 
    uint32_t cc = urgb_u32(0, 50, 50);   

    // **Matriz 5x5 do Sonic**
    uint32_t sonic[25] = {
        B,  B,  B,  B,  B,  
        B,  B,  B,  B,  B,  
        W,  W,  W,  W,  W,  
        cc,  cc,  W,  C,  C,  
        W,  W,  W,  W,  W
    };

    // **Enviar a matriz para os LEDs**
    for (int i = 0; i < NLEDS; i++) {
        fitaEd[i] = sonic[i];
    }

    atualizaFita();  // Atualiza os LEDs para exibir o Sonic
}


// **Função para desenhar o Pac-Man na Matriz de LED 5x5**
void draw_pacman() {

    // **Definição de Cores**
    uint32_t Y = urgb_u32(20, 20, 0);  
    uint32_t X = urgb_u32(0, 0, 0);      

    // **Matriz 5x5 do Pac-Man**
    uint32_t pacman[25] = {
        Y,  Y,  Y,  Y,  X,  
        Y,  Y,  Y,  X,  X,  
        X,  X,  X,  Y,  Y,  
        Y,  Y,  Y,  X,  X,  
        Y,  Y,  Y,  Y,  X
    };

    // **Enviar a matriz para os LEDs**
    for (int i = 0; i < NLEDS; i++) {
        fitaEd[i] = pacman[i];
    }

    atualizaFita();  // Atualiza os LEDs para exibir o Pac-Man
}

void draw_link() {
    
    uint32_t G = urgb_u32(0, 20, 0);   
    uint32_t Y = urgb_u32(20, 20, 0); 
    uint32_t B = urgb_u32(15, 10, 7); 
    uint32_t M = urgb_u32(18, 18, 4); 
    uint32_t X = urgb_u32(0, 0, 0);    

    uint32_t link[25] = {
        M,  M,  X,  M,  M,  
        G,  G,  G,  G,  G,  
        G,  G,  G,  G,  G,  
        X,  Y,  B,  B,  B,  
        G,  G,  G,  G,  G
    };

    for (int i = 0; i < NLEDS; i++) {
        fitaEd[i] = link[i];
    }

    atualizaFita();  
}
