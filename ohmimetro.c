#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "pico/bootrom.h"

/* ========== DEFINIÇÕES DE HARDWARE ========== */

// Configurações do Display OLED (I2C)
#define I2C_PORT i2c1      // Porta I2C utilizada
#define PIN_I2C_SDA 14     // Pino SDA (GPIO 14)
#define PIN_I2C_SCL 15     // Pino SCL (GPIO 15)
#define OLED_ADDRESS 0x3C  // Endereço I2C do display OLED

// Configurações do ADC (Voltímetro)
#define ADC_PIN 28         // GPIO para leitura analógica (GPIO 28)
#define botaoB 6           // GPIO para botão B (GPIO 6)

/* ========== VARIÁVEIS GLOBAIS ========== */

// Objeto do display OLED
ssd1306_t ssd;

// Configurações do circuito divisor de tensão para medição de resistência
int R_conhecido = 10000;   // Resistor conhecido (10k ohm) no divisor de tensão
float R_x = 0.0;           // Variável para armazenar a resistência desconhecida calculada
int ADC_RESOLUTION = 4095; // Resolução do ADC (12 bits)

// Variáveis para armazenar as cores dos anéis do resistor
const char* cor1;          // Cor do 1º anel (primeiro dígito)
const char* cor2;          // Cor do 2º anel (segundo dígito)
const char* cor3;          // Cor do 3º anel (multiplicador)

// Variáveis para cálculo dos dígitos do resistor
int primeiro_digito;       // Armazena o primeiro dígito significativo
int segundo_digito;        // Armazena o segundo dígito significativo
int multiplicador = 0;     // Armazena o multiplicador (potência de 10)

// Série E24 de valores padrão de resistores (valores comerciais comuns)
const int E24Series[] = {
  510, 560, 620, 680, 750, 820, 910,
  1000, 1100, 1200, 1300, 1500, 1600, 1800, 2000, 2200, 2400, 2700,
  3000, 3300, 3600, 3900, 4300, 4700, 5100, 5600, 6200, 6800, 7500, 8200, 9100,
  10000, 11000, 12000, 13000, 15000, 16000, 18000, 20000, 22000, 24000, 27000,
  30000, 33000, 36000, 39000, 43000, 47000, 51000, 56000, 62000, 68000, 75000,
  82000, 91000, 100000
};

/* ========== PROTÓTIPOS DE FUNÇÕES ========== */
void button_init(int button);                  // Inicializa o botão
void gpio_irq_handler(uint gpio, uint32_t events); // Handler de interrupção do botão
void display_init();                           // Inicializa o display OLED
const char* Seleciona_Cor(int numero);         // Retorna a cor correspondente ao código de resistor

/* ========== FUNÇÃO PRINCIPAL ========== */
int main()
{
  // Inicializações
  stdio_init_all();         // Inicializa comunicação serial (USB)
  button_init(botaoB);      // Configura o botão B
  
  // Configura interrupção para modo BOOTSEL (quando botão B é pressionado)
  gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

  display_init();           // Inicializa o display OLED
  
  // Configuração do ADC para leitura analógica
  adc_init();
  adc_gpio_init(ADC_PIN);   // Configura GPIO 28 como entrada analógica

  // Variáveis locais
  float tensao;             // Armazena a tensão lida (não utilizada atualmente)
  char str_x[5];            // Buffer para conversão de valores para string (ADC)
  char str_y[5];            // Buffer para conversão de valores para string (Resistência)

  bool cor = true;          // Variável para controle de cores no display

  // Loop principal
  while (true)
  {
    // Configura o ADC para ler do pino 28
    adc_select_input(2);
    
    // Realiza 500 leituras do ADC para obter uma média estável
    float soma = 0.0f;
    for (int i = 0; i < 500; i++)
    {
      soma += adc_read();
      sleep_ms(1);
    }
    float media = soma / 500.0f;

    // Calcula a resistência desconhecida usando a fórmula do divisor de tensão
    // R_x = (R_conhecido * V_out) / (V_in - V_out)
    // Como estamos lendo valores ADC, a fórmula é adaptada para:
    R_x = (R_conhecido * media) / (ADC_RESOLUTION - media);

    // Encontra o valor comercial mais próximo na série E24
    int resist_proxima = E24Series[0];
    int menor_diferenca = abs(R_x - E24Series[0]);
    int tamanho = sizeof(E24Series) / sizeof(E24Series[0]);
    
    for (int i = 1; i < tamanho; i++) {
        int diferenca = abs(R_x - E24Series[i]);
        if (diferenca < menor_diferenca) {
            menor_diferenca = diferenca;
            resist_proxima = E24Series[i];
        }
    }

    // Debug: imprime o valor mais próximo encontrado
    printf("Valor mais proximo: %f\n", R_x);

    // Extrai os dígitos significativos e o multiplicador do valor do resistor
    primeiro_digito = resist_proxima;  // Armazena o valor original para processamento
    segundo_digito = resist_proxima;   // Armazena o valor original para processamento
    
    // Extrai o primeiro dígito (divide por 10 até ficar com 1 dígito)
    while (primeiro_digito >= 10) {
        primeiro_digito /= 10;
    }
    
    // Extrai o segundo dígito e calcula o multiplicador
    while (segundo_digito >= 100) {
        segundo_digito /= 10;
        multiplicador++;
    }
    segundo_digito = segundo_digito % 10; // Pega apenas o segundo dígito

    // Obtém as cores correspondentes aos dígitos e multiplicador
    cor1 = Seleciona_Cor(primeiro_digito); // Cor do primeiro dígito
    cor2 = Seleciona_Cor(segundo_digito);  // Cor do segundo dígito
    cor3 = Seleciona_Cor(multiplicador);   // Cor do multiplicador

    // Debug: imprime os dígitos extraídos
    printf("%d  %d\n", primeiro_digito, segundo_digito);
    
    // Prepara strings para exibição no OLED
    sprintf(str_x, "%1.0f", media);  // Converte valor ADC para string
    sprintf(str_y, "%1.0f", R_x);    // Converte valor da resistência para string

    // Atualiza o display OLED
    ssd1306_fill(&ssd, !cor);                        // Limpa o display
    ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);   // Desenha borda
    ssd1306_line(&ssd, 3, 11, 123, 11, cor);        // Linha divisória 1
    ssd1306_line(&ssd, 3, 26, 123, 26, cor);        // Linha divisória 2
    ssd1306_line(&ssd, 3, 38, 123, 38, cor);        // Linha divisória 3
    ssd1306_line(&ssd, 3, 50, 123, 50, cor);        // Linha divisória 4
    
    // Exibe textos no display
    ssd1306_draw_string(&ssd, "OHMIMETRO", 28, 6);  // Título
    ssd1306_draw_string(&ssd, "OHMs:", 8, 16);      // Label resistência
    ssd1306_draw_string(&ssd, str_y, 55, 16);       // Valor resistência
    ssd1306_draw_string(&ssd, cor1, 8, 27);         // Cor 1º anel
    ssd1306_draw_string(&ssd, cor2, 8, 40);         // Cor 2º anel
    ssd1306_draw_string(&ssd, cor3, 8, 52);         // Cor multiplicador
    
    ssd1306_send_data(&ssd);                        // Atualiza o display
    sleep_ms(700);                                  // Delay para próxima leitura
    
    multiplicador = 0;  // Reseta o multiplicador para próxima iteração
  }
}

/* ========== IMPLEMENTAÇÃO DAS FUNÇÕES ========== */

// Inicializa um pino como entrada com pull-up
void button_init(int button)
{
  gpio_init(button);
  gpio_set_dir(button, GPIO_IN);
  gpio_pull_up(button);
}

// Handler de interrupção para o botão (entra em modo BOOTSEL quando pressionado)
void gpio_irq_handler(uint gpio, uint32_t events)
{
  reset_usb_boot(0, 0);  // Reinicia no modo BOOTSEL
}

// Inicializa o display OLED
void display_init()
{
  // Configura I2C a 400kHz
  i2c_init(I2C_PORT, 400 * 1000);
  
  // Configura pinos I2C
  gpio_set_function(PIN_I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(PIN_I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(PIN_I2C_SDA);
  gpio_pull_up(PIN_I2C_SCL);

  // Inicializa o display OLED
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, OLED_ADDRESS, I2C_PORT);
  ssd1306_config(&ssd);
  ssd1306_send_data(&ssd);
}

// Retorna a cor correspondente ao código de resistor
const char* Seleciona_Cor(int numero) {
  const char* cores[] = {
    "Preto", "Marrom", "Vermelho", "Laranja", "Amarelo","Verde", "Azul", "Violeta", "Cinza", "Branco"
  };
  
  if (numero >= 0 && numero <= 9) {
    return cores[numero];  // Retorna a cor correspondente
  } else {
    return "Erro";         // Retorna erro se número fora da faixa
  }
}