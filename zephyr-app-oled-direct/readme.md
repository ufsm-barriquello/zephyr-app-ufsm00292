# OLED1 Xplained Pro - Zephyr RTOS na SAMR21 Xplained Pro

## 📝 Descrição
Projeto de teste e integração do shield OLED1 Xplained Pro com a placa SAMR21 Xplained Pro utilizando Zephyr RTOS.

## 🎯 Objetivos Alcançados

### ✅ Funcionalidades Implementadas
- **SPI Interface**: SERCOM5 configurado para comunicação com display SSD1306
- **GPIO Control**: Controle completo dos 3 LEDs do OLED1
- **Button Input**: Leitura dos 3 botões com pull-up interno
- **Display Driver**: Driver SSD1306 inicializado via SPI
- **Character Framebuffer**: Biblioteca CFB pronta para uso

### ⚠️ Limitações Conhecidas
- Display físico não exibe conteúdo visual
- Possível incompatibilidade entre driver genérico SSD1306 do Zephyr e display UG-2832HSWEG04
- Necessita investigação adicional ou uso de biblioteca específica do fabricante

## 🔧 Hardware

### Placas Utilizadas
- **MCU Board**: SAMR21 Xplained Pro (ATSAMR21G18A)
- **Extension Board**: OLED1 Xplained Pro

### Conexões no EXT1
| Pin EXT1 | Função | GPIO SAMR21 | Descrição |
|----------|--------|-------------|-----------|
| 3 | BUTTON2 | PA06 | Botão 2 (active-low) |
| 4 | BUTTON3 | PA07 | Botão 3 (active-low) |
| 5 | DATA_CMD | PA13 | Data/Command select |
| 6 | LED3 | PA28 | LED3 (active-low) |
| 7 | LED1 | PA18 | LED1 (active-low) |
| 8 | LED2 | PA19 | LED2 (active-low) |
| 9 | BUTTON1 | PA22 | Botão 1 (active-low) |
| 10 | RESET | PA23 | Display Reset (active-low) |
| 15 | SPI_SS | PB03 | Chip Select |
| 16 | SPI_MOSI | PB22 | SPI Master Out |
| 17 | SPI_MISO | PB02 | SPI Master In |
| 18 | SPI_SCK | PB23 | SPI Clock |

## 📦 Estrutura do Projeto

```
zephyr-app-oled-direct/
├── src/
│   └── main.c              # Aplicação principal
├── app.overlay             # Device tree overlay
├── prj.conf                # Configurações do projeto
├── CMakeLists.txt          # Build configuration
└── README.md               # Esta documentação
```

## 🚀 Como Compilar

### Pré-requisitos
- Zephyr SDK 0.17.4
- West tool
- GCC ARM toolchain

### Comandos

```bash
# Compilar
west build -b samr21_xpro

# Flash na placa
west flash

# Monitorar saída serial
minicom -D /dev/ttyACM0 -b 115200
```

### Compilação limpa
```bash
west build -b samr21_xpro --pristine
```

## 🧪 Funcionalidades Testadas

### Teste de LEDs
No boot, os 3 LEDs acendem sequencialmente para validar a funcionalidade.

### Modo Interativo
- **BTN1** (SW1): Liga LED1 enquanto pressionado
- **BTN2** (SW2): Liga LED2 enquanto pressionado  
- **BTN3** (SW3): Liga LED3 enquanto pressionado

### Saída Serial
```
=============================================
 Teste Completo OLED1 Xplained Pro
=============================================

GPIO porta OK
LEDs e Botoes configurados

=== Teste de LEDs ===
LED1 ON...
LED2 ON...
LED3 ON...
Todos LEDs ON...
LEDs testados com sucesso!

=== Teste de Display ===
Display OK: ssd1306@0
CFB inicializado
Texto enviado

=== Modo Interativo ===
Pressione os botoes do OLED1:
  BTN1 (pin 9) -> LED1
  BTN2 (pin 3) -> LED2
  BTN3 (pin 4) -> LED3
```

## 📊 Configurações Importantes

### SPI (SERCOM5)
- Frequência: 4 MHz
- Modo: Master
- DIPO: 0, DOPO: 1

### Display SSD1306
- Resolução: 128x32 pixels
- Interface: SPI 4-wire
- Driver: solomon,ssd1306fb

### Memory Usage
```
FLASH: ~43 KB / 256 KB (17%)
RAM:   ~8 KB / 32 KB (25%)
```

## 🐛 Troubleshooting

### Display não mostra conteúdo
**Status**: Problema conhecido
**Causa provável**: Incompatibilidade entre driver genérico e display UG-2832HSWEG04
**Workaround**: Usar biblioteca específica do fabricante (Microchip ASF)

### LEDs não funcionam
**Verificar**: 
- Conexão firme do OLED1 no EXT1
- Orientação correta (pin 1 alinhado)
- Alimentação da placa

### Botões não respondem
**Verificar**:
- Pull-ups internos habilitados (já configurado no código)
- Conexão física do shield

## 🔬 Investigação Futura

Para resolver o problema do display:

1. **Opção A**: Portar biblioteca gfx_mono do Microchip ASF para Zephyr
2. **Opção B**: Investigar comandos de inicialização específicos do UG-2832HSWEG04
3. **Opção C**: Testar com outro display SSD1306 para isolar problema de hardware
4. **Opção D**: Usar samples oficiais do Zephyr como referência

## 📚 Referências

- [OLED1 Xplained Pro User Guide](https://ww1.microchip.com/downloads/en/DeviceDoc/40002176A.pdf)
- [SAMR21 Xplained Pro User Guide](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-42243-SAMR21-Xplained-Pro_User-Guide.pdf)
- [Zephyr Display Driver Documentation](https://docs.zephyrproject.org/latest/hardware/peripherals/display.html)
- [SSD1306 Driver Source](https://github.com/zephyrproject-rtos/zephyr/blob/main/drivers/display/ssd1306.c)

## 👥 Autor

Projeto desenvolvido como parte do curso de Sistemas Embarcados - UFSM

## 📄 Licença

Este projeto segue as licenças do Zephyr RTOS (Apache 2.0)
