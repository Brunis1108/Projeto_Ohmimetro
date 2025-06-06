## Ohmímetro Digital com Identificação de Cores de Resistores
### 📌 Objetivo
  - Desenvolver um sistema que mede automaticamente o valor de um resistor desconhecido, identifica o valor comercial mais próximo da série E24 e exibe as cores correspondentes das faixas do resistor.

### 🛠️ Componentes Principais
- Microcontrolador: RP2040 (BitDogLab)
- Display: OLED I2C
- Periféricos utilizados:
    - ADC (Conversor Analógico-Digital)
    - GPIO (Botão e comunicação I2C)
    - I2C (Display OLED SSD1306)

### 🔧 Funcionalidades
- Medição de Resistência:
    - Lê a tensão de um divisor resistivo (resistor conhecido + desconhecido)
    - Calcula o valor da resistência desconhecida usando a fórmula do divisor de tensão
- Identificação de Cores:
    - Encontra o valor comercial mais próximo na série E24
    - Extrai dígitos significativos e multiplicador
    - Exibe as cores correspondentes no display OLED
- Modo BOOTSEL:
    - Botão físico para forçar o RP2040 ao modo de bootloader

### 📋 Código
- Principais características:
    - Leitura de 500 amostras do ADC para reduzir ruído
    - Cálculo automático da resistência desconhecida
    - Exibição organizada no OLED (valores e cores)
    - Animações simples no display (bordas e linhas)

### 🔗 Links
<https://www.youtube.com/watch?v=GzmJ5bmd7BE>
