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
#define DEBOUNCE_TIME 200 

#define WS2812_PIN 7
#define NLEDS 25

static PIO pio;
static int sm;
static uint dma_chan;
static uint32_t fitaEd[NLEDS];

ssd1306_t ssd;
volatile uint32_t last_press_time_A = 0;
volatile uint32_t last_press_time_B = 0;

const char *games[] = {"Pokemon", "Sonic", "Pacman", "Zelda", "Undertale "};
const int num_games = 5;
int current_game = 0;

bool button_A_pressed = false;  
bool button_B_pressed = false;


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
                    ssd1306_fill(&ssd, false); 
                    /*ssd1306_draw_string(&ssd, " (\n--/) ___", 20, 5); 
                    ssd1306_draw_string(&ssd, " (o^.^) < / ", 20, 15); 
                    ssd1306_draw_string(&ssd, "  /  \n  / > ", 20, 25);  
                    ssd1306_draw_string(&ssd, " ( -  )> / ", 20, 35);
                    ssd1306_draw_string(&ssd, "  ^^ ^^ ", 20, 45);*/
                    ssd1306_draw_string(&ssd, "\n _.__._/.-\"", 2, 2);
                    ssd1306_draw_string(&ssd, " \n         /      ,", 2, 10);
                    ssd1306_draw_string(&ssd, " /()   ()  \n    .' -._", 2, 16);
                    ssd1306_draw_string(&ssd, " |)   .   ()\n  /   _.'", 2, 24);
                    ssd1306_draw_string(&ssd, " \n\n  ---     ,; '. <", 2, 32);
                    ssd1306_draw_string(&ssd, "  ;.__     ,;|   > \n", 2, 40);
                    ssd1306_draw_string(&ssd, " / ,    / ,  |.-'.-'", 2, 48);
                    ssd1306_draw_string(&ssd, "(_/    (_/ ,;|.<", 2, 56);
                    ssd1306_send_data(&ssd);
                    draw_pokeball();
                    play_Pokemon(BUZZER_A);
                    break;
                case 1:
                    ssd1306_fill(&ssd, false);
                    ssd1306_draw_string(&ssd, " |\n\n ,-'\'/  |'", 2, 2);
                    ssd1306_draw_string(&ssd, " |,_  ,--.    /", 2, 10);
                    ssd1306_draw_string(&ssd, "/,-. ,''.   (_", 2, 18);
                    ssd1306_draw_string(&ssd, "(  o|  o)__   \''-.", 2, 24);
                    ssd1306_draw_string(&ssd, ",-._.,--'_ '. _.,-'", 2, 32);
                    ssd1306_draw_string(&ssd, "\n\'' ___.,'' /,'", 2, 40);
                    ssd1306_draw_string(&ssd, "  \n-.__.,--'", 2, 48);
                    ssd1306_send_data(&ssd);
                    draw_sonic();
                    play_Sonic(BUZZER_A);
                    break;
                case 2:
                    ssd1306_fill(&ssd, false);
                    ssd1306_draw_string(&ssd, " .-.  .-.  ---.  ", 2, 2);
                    ssd1306_draw_string(&ssd, "| OO|| OO|/  /' ", 2, 10);
                    ssd1306_draw_string(&ssd, "|   ||   |\n  \n. ", 2, 16);
                    ssd1306_draw_string(&ssd, "'^^^''^^^' ---'  ", 2, 24);
                    ssd1306_draw_string(&ssd, "===============", 2, 32);
                    ssd1306_draw_string(&ssd, ".-. .-.  .--. | ", 2, 40);
                    ssd1306_draw_string(&ssd, "'-' '-'  '..' |", 2, 48);
                    ssd1306_draw_string(&ssd, "===============", 2, 54);
                    ssd1306_send_data(&ssd);
                    draw_pacman();
                    play_Pacman(BUZZER_A);
                    break;
                case 3:
                    ssd1306_fill(&ssd, false);
                    ssd1306_draw_string(&ssd, "|             |", 0, 0);
                    ssd1306_draw_string(&ssd, "\n .--.|__|.--./", 0, 8);
                    ssd1306_draw_string(&ssd, " |'--'/  \n'--'|", 0, 16);
                    ssd1306_draw_string(&ssd, " |'  /____\n  '|", 0, 24);
                    ssd1306_draw_string(&ssd, " | .' .''. '. |", 0, 32);
                    ssd1306_draw_string(&ssd, "/   |_/  \n_|  \n", 0, 40);
                    ssd1306_draw_string(&ssd, "|             |", 0, 48);
                    ssd1306_send_data(&ssd);
                    draw_link();
                    play_Zelda(BUZZER_A);
                    break;
                case 4:
                    ssd1306_fill(&ssd, false);
                    ssd1306_draw_string(&ssd, "  .o0000000o.", 2, 2);
                    ssd1306_draw_string(&ssd, " |0000000000o|", 2, 10);
                    ssd1306_draw_string(&ssd, "|00^^^1^^^1001.", 2, 18);
                    ssd1306_draw_string(&ssd, "00' ()  () 0670", 2, 26);
                    ssd1306_draw_string(&ssd, "00  ''  '' 7660", 2, 34);
                    ssd1306_draw_string(&ssd, "9P0oc_   lo0P90", 2, 42);
                    ssd1306_draw_string(&ssd, "'''^^^1__l8P^l'", 2, 50);
                    ssd1306_send_data(&ssd);
                    draw_sans(); 
                    play_Megalovania(BUZZER_A); 
                    break;
                }
        }
        sleep_ms(200);
    }   

    return 0;
}


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


void setup_adc() {
    adc_init();
    adc_gpio_init(26);
    adc_gpio_init(27);
}


void pwm_init_buzzer(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_set_enabled(slice_num, true);
}


void setup_i2c() {
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}


void setup_ss1306() {
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, SSD1306_ADDR, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}


void ws2812_init() {
    pio = pio0;
    sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, false);
    dma_chan = dma_claim_unused_channel(true);
}


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

void update_display() {
    ssd1306_fill(&ssd, false);
    ssd1306_rect(&ssd, 0, 0, 128, 62, true, false); ssd1306_rect(&ssd, 2, 2, 124, 58, true, false);
    ssd1306_draw_string(&ssd, games[current_game], 40, 28);
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


inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(g) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(b) << 8);
}


void draw_pokeball() {
    uint32_t R = urgb_u32(3, 0, 0);   
    uint32_t W = urgb_u32(3, 3, 3); 

    uint32_t pokeball[5][5] = {
        {0,  W,  W,  W,  0},
        {W,  W,  W,  W,  W},
        {W,  W,  0,  W,  W},
        {R,  R,  R,  R,  R},
        {0,  R,  R,  R,  0}
    };

    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            int index = y * 5 + x;
            fitaEd[index] = pokeball[y][x]; 
        }
    }

    atualizaFita(); 
}


void draw_sonic() {
    
    uint32_t B = urgb_u32(0, 0, 10);   
    uint32_t W = urgb_u32(10, 10, 10);
    uint32_t R = urgb_u32(10, 0, 0);   
    uint32_t X = urgb_u32(0, 0, 0);
    uint32_t P = urgb_u32(10, 0, 10);      

    
    uint32_t sonic[25] = {
        X,  R,  R,  R,  R,  
        X,  B,  B,  B,  X,  
        B,  B,  B,  B,  B,  
        W,  W,  B,  B,  X,  
        B,  B,  B,  B,  B
    };

    
    for (int i = 0; i < NLEDS; i++) {
        fitaEd[i] = sonic[i];
    }

    atualizaFita();  
}


void draw_sans() {
    
    uint32_t B = urgb_u32(0, 0, 3);   
    uint32_t W = urgb_u32(3, 3, 3);
    uint32_t C = urgb_u32(0, 70, 70); 
    uint32_t cc = urgb_u32(0, 50, 50);   

    
    uint32_t sonic[25] = {
        B,  B,  B,  B,  B,  
        B,  B,  B,  B,  B,  
        W,  W,  W,  W,  W,  
        cc,  cc,  W,  C,  C,  
        W,  W,  W,  W,  W
    };

    
    for (int i = 0; i < NLEDS; i++) {
        fitaEd[i] = sonic[i];
    }

    atualizaFita();  
}



void draw_pacman() {

    
    uint32_t Y = urgb_u32(20, 20, 0);  
    uint32_t X = urgb_u32(0, 0, 0);      

    
    uint32_t pacman[25] = {
        Y,  Y,  Y,  Y,  X,  
        Y,  Y,  Y,  X,  X,  
        X,  X,  X,  Y,  Y,  
        Y,  Y,  Y,  X,  X,  
        Y,  Y,  Y,  Y,  X
    };

    
    for (int i = 0; i < NLEDS; i++) {
        fitaEd[i] = pacman[i];
    }

    atualizaFita();  
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
