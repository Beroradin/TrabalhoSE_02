/* Código Realizado como parte do Trabalho 02 da trilha de Sistemas Embarcados
do programa Embarcatech. O projeto consiste no aperfeiçoamento de um ohmímetro
como por exemplo, exibir o valor comercial e as faixas de cores do resistor medido.
*/

// Bibliotecas utilizadas
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <math.h>
 #include "pico/stdlib.h"
 #include "hardware/adc.h"
 #include "hardware/i2c.h"
 #include "hardware/timer.h"
 #include "lib/ssd1306.h"
 #include "lib/font.h"
 #include "pico/bootrom.h"

 // Configurações utilizadas
 #define I2C_PORT i2c1
 #define I2C_SDA 14
 #define I2C_SCL 15
 #define endereco 0x3C
 #define ADC_PIN 28 // GPIO para o voltímetro
 #define Botao_A 5  // GPIO para botão A
 #define Botao_B 6   // GPIO para botão B
 
 // Variáveis globais
 int R_conhecido = 9730;    // Resistor de 10k ohm (foi utilizado um resistor de 9k73 previamente medido com multímetro)
 float R_x = 0.0;           // Resistor desconhecido
 float ADC_VREF = 3.31;     // Tensão de referência do ADC
 int ADC_RESOLUTION = 4095; // Resolução do ADC 12 bits (2^12 - 1 = 4095)
 float valor_comercial = 0.0; // Valor comercial encontrado
 
 // Modo de exibição: 0 = resistência, 1 = cores, 2 = valor comercial
 int modo_exibicao = 0;
 absolute_time_t last_interrupt_time = 0; // Variável para armazenar o tempo da última interrupção
 
 // Estrutura para armazenar as cores dos resistores
 typedef struct {
     char *nome;
     int valor;
 } Cor;
 
 // Definição das cores dos resistores
 const Cor cores[] = {
     {"Preto", 0},
     {"Marrom", 1},
     {"Vermelho", 2},
     {"Laranja", 3},
     {"Amarelo", 4},
     {"Verde", 5},
     {"Azul", 6},
     {"Violeta", 7},
     {"Cinza", 8},
     {"Branco", 9}
 };
 
 // Valores comerciais padrão de resistores
 const float valores_comerciais[] = {
     1.0, 1.2, 1.5, 1.8, 2.2, 2.7, 3.3, 3.6, 3.9, 4.7, 5.6, 6.8, 8.2, 10.0,
     12.0, 15.0, 18.0, 22.0, 27.0, 33.0, 36.0, 39.0, 47.0, 56.0, 68.0, 82.0, 100.0,
     120.0, 150.0, 180.0, 220.0, 270.0, 330.0, 360.0, 390.0, 470.0, 560.0, 680.0, 820.0, 1000.0,
     1200.0, 1500.0, 1800.0, 2200.0, 2700.0, 3300.0, 3600.0, 3900.0, 4700.0, 5600.0, 6800.0, 8200.0, 10000.0,
     12000.0, 15000.0, 18000.0, 22000.0, 27000.0, 33000.0, 36000.0, 39000.0, 47000.0, 56000.0, 68000.0, 82000.0, 100000.0,
     120000.0, 150000.0, 180000.0, 220000.0, 270000.0, 330000.0, 390000.0, 470000.0, 560000.0, 680000.0, 820000.0
 };

 const int num_valores_comerciais = sizeof(valores_comerciais) / sizeof(valores_comerciais[0]); // Número de valores comerciais
 
 // Protótipos das funções
 void gpio_irq_handler(uint gpio, uint32_t events);
 void obter_cores_resistor(float resistencia, char *cor1, char *cor2, char *cor_mult);
 float encontrar_valor_comercial(float resistencia);
 void formatar_resistencia(float valor, char *buffer);
 
 int main(void)
 {
   // Modo BootSEL por meio do botão B (muito útil)
   gpio_init(Botao_B);
   gpio_set_dir(Botao_B, GPIO_IN);
   gpio_pull_up(Botao_B);
   
   // Configuração do botão A para alternar entre os menus do display
   gpio_init(Botao_A);
   gpio_set_dir(Botao_A, GPIO_IN);
   gpio_pull_up(Botao_A);

   // Configurração das interrupções
   gpio_set_irq_enabled(Botao_A, GPIO_IRQ_EDGE_FALL, true);
   gpio_set_irq_enabled(Botao_B, GPIO_IRQ_EDGE_FALL, true);
   gpio_set_irq_callback(gpio_irq_handler);
   irq_set_enabled(IO_IRQ_BANK0, true);
 
   // Inicializa a serial para depuração
   stdio_init_all();
   sleep_ms(2000);  // Aguarda a inicialização da serial
   printf("Ohmímetro iniciado\n");
 
   // Inicialização do I2C usando 400KHz.
   i2c_init(I2C_PORT, 400 * 1000);
 
   gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
   gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
   gpio_pull_up(I2C_SDA);                                        // Pull up the data line
   gpio_pull_up(I2C_SCL);                                        // Pull up the clock line
   ssd1306_t ssd;                                                // Inicializa a estrutura do display
   ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
   ssd1306_config(&ssd);                                         // Configura o display
   ssd1306_send_data(&ssd);                                      // Envia os dados para o display
 
   // Limpa o display. O display inicia com todos os pixels apagados.
   ssd1306_fill(&ssd, false);
   ssd1306_send_data(&ssd);
 
   adc_init();
   adc_gpio_init(ADC_PIN); // GPIO 28 como entrada analógica
 
   // Alguns buffers
   float tensao;
   char str_x[10];    // Buffer para ADC
   char str_y[10];    // Buffer para resistência
   char cor1[20], cor2[20], cor_mult[20]; // Buffers para as cores
   char valor_formatado[15]; // Para mostrar o valor formatado
   char valor_comercial_str[15]; // Para mostrar o valor comercial formatado
   
   bool cor = true;
   while (true)
   {
     adc_select_input(2);
 
     float soma = 0.0f;
     for (int i = 0; i < 500; i++)
     {
       soma += adc_read();
       sleep_ms(1);
     }
     float media = soma / 500.0f;
 
     // Fórmula simplificada do divisor de tensão: R_x = R_conhecido * ADC_encontrado /(ADC_RESOLUTION - adc_encontrado)
     R_x = (R_conhecido * media) / (ADC_RESOLUTION - media);
 
     sprintf(str_x, "%1.0f", media); // Converte o inteiro em string
     sprintf(str_y, "%1.0f", R_x);   // Converte o float em string
 
     // Formata o valor da resistência de forma mais legível
     formatar_resistencia(R_x, valor_formatado);
     
     // Encontra o valor comercial mais próximo
     valor_comercial = encontrar_valor_comercial(R_x);
     formatar_resistencia(valor_comercial, valor_comercial_str);
 
     // Obtém as cores do resistor BASEADO NO VALOR COMERCIAL
     obter_cores_resistor(valor_comercial, cor1, cor2, cor_mult);
 
     // Atualiza o conteúdo do display
     ssd1306_fill(&ssd, !cor);                          // Limpa o display
     ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);      // Desenha um retângulo
     
     if (modo_exibicao == 1) {
       // Exibe o display com as cores do resistor BASEADAS NO VALOR COMERCIAL
       ssd1306_draw_string(&ssd, "CEPEDI   TIC37", 8, 6);        // Desenha uma string
       ssd1306_draw_string(&ssd, "EMBARCATECH", 20, 16);         // Desenha uma string
       ssd1306_line(&ssd, 3, 25, 123, 25, cor);                  // Desenha uma linha
       ssd1306_draw_string(&ssd, "Cores Resistor", 8, 28);       // Desenha uma string
       ssd1306_line(&ssd, 3, 37, 123, 37, cor);                  // Desenha uma linha
       
       // Exibe as três cores baseadas no valor comercial
       ssd1306_draw_string(&ssd, "1.", 5, 40);
       ssd1306_draw_string(&ssd, cor1, 22, 40);
       
       ssd1306_draw_string(&ssd, "2.", 5, 47);
       ssd1306_draw_string(&ssd, cor2, 22, 47);
       
       ssd1306_draw_string(&ssd, "3.", 5, 54);
       if (cor_mult[0] != '\0') {
         ssd1306_draw_string(&ssd, cor_mult, 22, 54);
       } else {
         ssd1306_draw_string(&ssd, "N/A", 22, 54);
       }
     } else if (modo_exibicao == 2) {
       // Exibe o display com valor comercial
       ssd1306_draw_string(&ssd, "CEPEDI   TIC37", 8, 6);        // Desenha uma string
       ssd1306_draw_string(&ssd, "EMBARCATECH", 20, 16);         // Desenha uma string
       ssd1306_line(&ssd, 3, 25, 123, 25, cor);                  // Desenha uma linha
       ssd1306_draw_string(&ssd, "Val. Comercial", 6, 28);      // Desenha uma string
       ssd1306_line(&ssd, 3, 37, 123, 37, cor);                  // Desenha uma linha
       
       // Exibe os valores medidos e comerciais
       ssd1306_draw_string(&ssd, "Medido:", 6, 42);
       ssd1306_draw_string(&ssd, valor_formatado, 82, 42);
       
       ssd1306_draw_string(&ssd, "Comercial:", 6, 52);
       ssd1306_draw_string(&ssd, valor_comercial_str, 82, 52);
       
     } else {
       // Modo 0: Exibe o display padrão com resistência
       ssd1306_line(&ssd, 3, 25, 123, 25, cor);           // Desenha uma linha
       ssd1306_line(&ssd, 3, 37, 123, 37, cor);           // Desenha uma linha
       ssd1306_draw_string(&ssd, "CEPEDI   TIC37", 8, 6); // Desenha uma string
       ssd1306_draw_string(&ssd, "EMBARCATECH", 20, 16);  // Desenha uma string
       ssd1306_draw_string(&ssd, "  Ohmimetro", 10, 28);  // Desenha uma string
       ssd1306_draw_string(&ssd, "ADC", 13, 41);          // Desenha uma string
       ssd1306_draw_string(&ssd, "Resisten.", 50, 41);    // Desenha uma string
       ssd1306_line(&ssd, 44, 37, 44, 60, cor);           // Desenha uma linha vertical
       ssd1306_draw_string(&ssd, str_x, 8, 52);           // Desenha uma string
       ssd1306_draw_string(&ssd, str_y, 59, 52);          // Desenha uma string
     }
     
     ssd1306_send_data(&ssd);    // Atualiza o display
     sleep_ms(700);
   }
 }


  // Implementação das função de interrupção e o debounce por software
  void gpio_irq_handler(uint gpio, uint32_t events)
  {
     absolute_time_t now = get_absolute_time();
     int64_t diff = absolute_time_diff_us(last_interrupt_time, now);
  
     if (diff < 250000) return;
     last_interrupt_time = now;
     
     if (gpio == Botao_B) {
       reset_usb_boot(0, 0);
     } else if (gpio == Botao_A) {
       // Alterna entre os três modos: resistência, cores, valor comercial
       modo_exibicao = (modo_exibicao + 1) % 3;
     }
  }
   
  // Função para determinar as cores do resistor baseado no valor comercial
  void obter_cores_resistor(float resistencia, char *cor1, char *cor2, char *cor_mult) {
      // Para evitar problemas com valores muito pequenos
      if (resistencia < 1.0) {
          strcpy(cor1, "N/A");
          strcpy(cor2, "N/A");
          strcpy(cor_mult, "N/A");
          return;
      }
      
      // Determinar o multiplicador (potência de 10)
      int multiplicador = 0;
      float temp = resistencia;
      
      // Normalizar para um número entre 10 e 99
      while (temp >= 100) {
          temp /= 10;
          multiplicador++;
      }
      
      while (temp < 10) {
          temp *= 10;
          multiplicador--;
      }
      
      // Obter os dois primeiros dígitos
      int primeiro_digito = (int)(temp / 10);
      int segundo_digito = (int)(temp) % 10;
      
      // Atribuir as cores correspondentes
      strcpy(cor1, cores[primeiro_digito].nome);
      strcpy(cor2, cores[segundo_digito].nome);
      
      // Atribuir a cor do multiplicador
      if (multiplicador >= 0 && multiplicador < 10) {
          strcpy(cor_mult, cores[multiplicador].nome);
      } else if (multiplicador == -1) {
          strcpy(cor_mult, "Dourado"); // 0.1
      } else if (multiplicador == -2) {
          strcpy(cor_mult, "Prata");   // 0.01
      } else {
          strcpy(cor_mult, "N/A");
      }
      
      //
      printf("###############################\n");
      printf("Valor comercial: %.2f ohms\n", resistencia);
      printf("Dígitos: %d%d × 10^%d\n", primeiro_digito, segundo_digito, multiplicador);
      printf("Cores: %s, %s, %s\n", cor1, cor2, cor_mult);
  }
  
  // Função para encontrar o valor comercial mais próximo
  float encontrar_valor_comercial(float resistencia) {
      float valor_mais_proximo = valores_comerciais[0];
      float menor_diferenca = fabs(resistencia - valores_comerciais[0]);
      // Algoritmo de busca linear simples
      for (int i = 1; i < num_valores_comerciais; i++) {
          float diferenca = fabs(resistencia - valores_comerciais[i]);
          if (diferenca < menor_diferenca) {
              menor_diferenca = diferenca;
              valor_mais_proximo = valores_comerciais[i];
          }
      }
      
      // Imprimir para depuração
      printf("Valor medido: %.2f, Valor comercial mais próximo: %.2f\n", resistencia, valor_mais_proximo);
      
      return valor_mais_proximo;
  }
  
  // Função para formatar valores de resistência
  void formatar_resistencia(float valor, char *buffer) {
      if (valor >= 1000) {
          sprintf(buffer, "%.1fk", valor / 1000.0);
      } else {
          sprintf(buffer, "%.1f", valor);
      }
  }