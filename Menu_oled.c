//Tarefa 6.2 - Desenvolvimento de uma Solução em Sistemas Embarcados
//Autor: Carlos Henrique Dantas da Costa 

//Bibliotecas 
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

//Declaração das variáveis
#define I2C_PORT i2c1
#define I2C_SDA 15
#define I2C_SCL 14
#define LED_B 12
#define LED_R 13
#define LED_G 11
#define SW 22  
#define VRY 26 
#define VRX 27
#define BUZZER_PIN 21

const int ADC_CHANNEL_0 = 0;
const int ADC_CHANNEL_1 = 1;
const float DIVIDER_PWM = 16.0;
const uint16_t PERIOD = 4096;
const uint16_t PERIOD1= 2000;   // Período do PWM (valor máximo do contador)
const uint16_t LED_STEP = 100;  // Passo de incremento/decremento para o duty cycle do LED
uint16_t led_level = 100;       // Nível inicial do PWM (duty cycle)
uint16_t led_b_level, led_r_level = 100;
uint slice_led_b, slice_led_r;

#define JOYSTICK_CENTER 2048  
#define JOYSTICK_THRESHOLD 500 

ssd1306_t disp;


//Funções 
void setup() {
    stdio_init_all();
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    disp.external_vcc = false;
    ssd1306_init(&disp, 128, 64, 0x3C, I2C_PORT);
    ssd1306_clear(&disp);
    gpio_init(LED_B);
    gpio_init(LED_R);
    gpio_init(LED_G);
    gpio_set_dir(LED_B, GPIO_OUT);
    gpio_set_dir(LED_R, GPIO_OUT);
    gpio_set_dir(LED_G, GPIO_OUT);
    gpio_init(SW);
    gpio_set_dir(SW, GPIO_IN);
    gpio_pull_up(SW);
    adc_init();
    adc_gpio_init(VRY);
    adc_gpio_init(VRX);
}

void print_texto(int x, int y, int tam, char *msg) {
    ssd1306_draw_string(&disp, x, y, tam, msg);
    ssd1306_show(&disp);
}

void print_retangulo(int x1, int y1, int x2, int y2) {
    ssd1306_draw_empty_square(&disp, x1, y1, x2, y2);
    ssd1306_show(&disp);
}

void print_menu(int pos) {
    ssd1306_clear(&disp);
    print_texto(52, 2, 1, "Menu");
    print_retangulo(2, pos + 2, 120, 12);
    print_texto(6, 18, 1.5, "Programa 01");
    print_texto(6, 30, 1.5, "Programa 02");
    print_texto(6, 42, 1.5, "Programa 03");
}

void setup_pwm_led(uint led, uint *slice, uint16_t level) {
    gpio_set_function(led, GPIO_FUNC_PWM);
    *slice = pwm_gpio_to_slice_num(led);
    pwm_set_clkdiv(*slice, DIVIDER_PWM);
    pwm_set_wrap(*slice, PERIOD);
    pwm_set_gpio_level(led, level);
    pwm_set_enabled(*slice, true);
}

void setup_programa1() {
    setup_pwm_led(LED_B, &slice_led_b, led_b_level);
    setup_pwm_led(LED_R, &slice_led_r, led_r_level);
}

//Programa 2
// Notas musicais para a música tema de Star Wars
const uint star_wars_notes[] = {
    330, 330, 330, 262, 392, 523, 330, 262,
    392, 523, 330, 659, 659, 659, 698, 523,
    415, 349, 330, 262, 392, 523, 330, 262,
    392, 523, 330, 659, 659, 659, 698, 523,
    415, 349, 330, 523, 494, 440, 392, 330,
    659, 784, 659, 523, 494, 440, 392, 330,
    659, 659, 330, 784, 880, 698, 784, 659,
    523, 494, 440, 392, 659, 784, 659, 523,
    494, 440, 392, 330, 659, 523, 659, 262,
    330, 294, 247, 262, 220, 262, 330, 262,
    330, 294, 247, 262, 330, 392, 523, 440,
    349, 330, 659, 784, 659, 523, 494, 440,
    392, 659, 784, 659, 523, 494, 440, 392
};

// Duração das notas em milissegundos
const uint note_duration[] = {
    500, 500, 500, 350, 150, 300, 500, 350,
    150, 300, 500, 500, 500, 500, 350, 150,
    300, 500, 500, 350, 150, 300, 500, 350,
    150, 300, 650, 500, 150, 300, 500, 350,
    150, 300, 500, 150, 300, 500, 350, 150,
    300, 650, 500, 350, 150, 300, 500, 350,
    150, 300, 500, 500, 500, 500, 350, 150,
    300, 500, 500, 350, 150, 300, 500, 350,
    150, 300, 500, 350, 150, 300, 500, 500,
    350, 150, 300, 500, 500, 350, 150, 300,
};

// Inicializa o PWM no pino do buzzer
void pwm_init_buzzer(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f); // Ajusta divisor de clock
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(pin, 0); // Desliga o PWM inicialmente
}

// Toca uma nota com a frequência e duração especificadas
void play_tone(uint pin, uint frequency, uint duration_ms) {
    uint slice_num = pwm_gpio_to_slice_num(pin);
    uint32_t clock_freq = clock_get_hz(clk_sys);
    uint32_t top = clock_freq / frequency - 1;

    pwm_set_wrap(slice_num, top);
    pwm_set_gpio_level(pin, top / 2); // 50% de duty cycle

    sleep_ms(duration_ms);

    pwm_set_gpio_level(pin, 0); // Desliga o som após a duração
    sleep_ms(50); // Pausa entre notas
}

// Função principal para tocar a música
void play_star_wars(uint pin) {
    for (int i = 0; i < sizeof(star_wars_notes) / sizeof(star_wars_notes[0]); i++) {
        if (star_wars_notes[i] == 0) {
            sleep_ms(note_duration[i]);
        } else {
            play_tone(pin, star_wars_notes[i], note_duration[i]);
        }
    }
}

//Programa 3
void setup_pwm()
{
    uint slice;
    gpio_set_function(LED_B, GPIO_FUNC_PWM); // Configura o pino do LED para função PWM
    slice = pwm_gpio_to_slice_num(LED_B);    // Obtém o slice do PWM associado ao pino do LED
    pwm_set_clkdiv(slice, DIVIDER_PWM);    // Define o divisor de clock do PWM
    pwm_set_wrap(slice, PERIOD1);           // Configura o valor máximo do contador (período do PWM)
    pwm_set_gpio_level(LED_B, led_level);    // Define o nível inicial do PWM para o pino do LED
    pwm_set_enabled(slice, true);          // Habilita o PWM no slice correspondente
}

void joystick_read_axis(uint16_t *vrx_value, uint16_t *vry_value) {
    adc_select_input(ADC_CHANNEL_0);
    sleep_us(2);
    *vrx_value = adc_read();
    adc_select_input(ADC_CHANNEL_1);
    sleep_us(2);
    *vry_value = adc_read();
}

void desligar_leds() {
    pwm_set_gpio_level(LED_B, 0);
    pwm_set_gpio_level(LED_R, 0);
    pwm_set_enabled(slice_led_b, false);
    pwm_set_enabled(slice_led_r, false);
    
    gpio_set_function(LED_B, GPIO_FUNC_SIO);
    gpio_set_function(LED_R, GPIO_FUNC_SIO);
    gpio_set_dir(LED_B, GPIO_OUT);
    gpio_set_dir(LED_R, GPIO_OUT);
    gpio_put(LED_B, 0);
    gpio_put(LED_R, 0);
    gpio_put(LED_G, 0);
    
}

void executar_programa(int programa) {
    int executando = 1;
    while (executando) {
        switch (programa) {
            case 1:
                ssd1306_clear(&disp);
                print_texto(10, 30, 1.5, "Executando P1...");
                ssd1306_show(&disp);
                uint16_t vrx_value, vry_value;
                setup_programa1();
                while (1) {
                    if (gpio_get(SW) == 0) {
                        while (gpio_get(SW) == 0);  // Espera o botão ser solto
                        executando = 0;
                        desligar_leds();
                        break;
                    }
                    joystick_read_axis(&vrx_value, &vry_value);
                    pwm_set_gpio_level(LED_B, vrx_value);
                    pwm_set_gpio_level(LED_R, vry_value);
                    sleep_ms(100);
                }
                break;

             case 2:
                ssd1306_clear(&disp);
                print_texto(10, 30, 1.5, "Executando P2...");
                ssd1306_show(&disp);
                pwm_init_buzzer(BUZZER_PIN);
                
                int interrompeu = 0; // Flag para saber se o programa foi interrompido pelo botão

                // Toca a música
                for (int i = 0; i < sizeof(star_wars_notes) / sizeof(star_wars_notes[0]); i++) {
                    // Verifica se o botão foi pressionado
                    if (gpio_get(SW) == 0) {
                        while (gpio_get(SW) == 0); // Espera o botão ser solto
                        interrompeu = 1; // Marca que a execução foi interrompida
                        break; // Sai do loop de notas
                    }
                    
                    // Toca a nota atual
                    if (star_wars_notes[i] == 0) {
                        sleep_ms(note_duration[i]);
                    } else {
                        play_tone(BUZZER_PIN, star_wars_notes[i], note_duration[i]);
                    }
                }

                // Se o programa foi interrompido pelo botão
                if (interrompeu) {
                    executando = 0;  // Interrompe o programa
                    desligar_leds(); // Desliga LEDs e buzzer
                }

                break;

            case 3:
                ssd1306_clear(&disp);
                print_texto(10, 30, 1.5, "Executando P3...");
                ssd1306_show(&disp);
                
                uint up_down = 1; // Variável para controlar se o nível do LED aumenta ou diminui
                stdio_init_all(); // Inicializa o sistema padrão de I/O
                setup_pwm();      // Configura o PWM
                
                led_level = 100;  // Garantir que o LED comece com um nível de PWM adequado (ajuste conforme necessário)
                pwm_set_gpio_level(LED_B, led_level); // Inicializa o LED azul com um valor inicial
            
                uint last_sw_state = gpio_get(SW);  // Variável para armazenar o último estado do botão
                uint sw_state = last_sw_state; // Variável atual do estado do botão
            
                while (executando) { // Usa a variável "executando" para controlar o loop
                    pwm_set_gpio_level(LED_B, led_level); // Define o nível atual do PWM (duty cycle)
                    sleep_ms(100);  // Atraso pequeno para evitar sobrecarga no processador
            
                    // Verifica se o botão foi pressionado (verificando a mudança de estado do botão)
                    sw_state = gpio_get(SW);
                    if (sw_state == 0 && last_sw_state == 1) {  // Detecta a transição do botão
                        sleep_ms(50); // Debounce do botão
                        if (gpio_get(SW) == 0) { // Verifica novamente para garantir que o botão ainda está pressionado
                            executando = 0; // Interrompe o loop
                            desligar_leds(); // Apaga os LEDs
                            break; // Sai do loop e retorna ao menu
                        }
                    }
                    last_sw_state = sw_state; // Atualiza o estado do botão
            
                    // Controle do aumento e diminuição do LED
                    if (up_down) {
                        led_level += LED_STEP; // Incrementa o nível do LED
                        if (led_level >= PERIOD1)
                            up_down = 0; // Muda direção para diminuir quando atingir o período máximo
                    } else {
                        led_level -= LED_STEP; // Decrementa o nível do LED
                        if (led_level <= LED_STEP)
                            up_down = 1; // Muda direção para aumentar quando atingir o mínimo
                    }
                }
                break;
                
        }
        sleep_ms(100);  
    }
    print_menu(12); 
}


int main() {
    setup();
    uint pos_y = 12;
    uint posy_ant = 12;
    uint menu = 1;
    print_menu(pos_y);
    while (true) {
        adc_select_input(0);
        uint adc_y_raw = adc_read();
        if (adc_y_raw < (JOYSTICK_CENTER - JOYSTICK_THRESHOLD)) {
            if (menu < 3) {
                pos_y += 12;
                menu++;
            }
        } else if (adc_y_raw > (JOYSTICK_CENTER + JOYSTICK_THRESHOLD)) {
            if (menu > 1) {
                pos_y -= 12;
                menu--;
            }
        }
        if (pos_y != posy_ant) {
            print_menu(pos_y);
            posy_ant = pos_y;
        }
        if (gpio_get(SW) == 0) {
            while (gpio_get(SW) == 0);
            executar_programa(menu);
            print_menu(pos_y);
        }
        sleep_ms(150);
    }
}
