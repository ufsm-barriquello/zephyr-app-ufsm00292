# Driver Zephyr RTOS para Sensor AT30TSE752A 

**Disciplina:** UFSM00292 - Projeto de Sistemas Embarcados (2025/2)  
**Professor:** Carlos Henrique Barriquello  
**Equipe:** HAL - Desenvolvimento de Drivers  
**Desenvolvedora:** Gabriela Bernardoni de Vasconcelos

---

## Índice
- [Sumário Executivo](#-sumário-executivo)
- [Como Funciona](#-como-funciona-o-driver)
- [Objetivos do Projeto](#-objetivos-do-projeto) 
- [Cronograma](#-cronograma-de-8-semanas)
- [Implementação Técnica](#-implementação-técnica)
- [Resultados e Validação](#-resultados-e-validação)
- [Como Usar](#-como-usar-o-driver)
- [Aprendizados](#-aprendizados-e-desafios)

---

## Sumário Executivo

Este projeto consiste no desenvolvimento completo de um driver para o sensor de temperatura **AT30TSE752A** da Microchip, integrado ao ecossistema **Zephyr RTOS** e validado nas placas **SAM D21 Xplained Pro** e **SAM R21 Xplained Pro**.

> **STATUS: PROJETO CONCLUÍDO COM SUCESSO**

** Métricas Principais:**
-  **100% dos objetivos** alcançados
-  **8 semanas** de desenvolvimento
-  **Driver 100% funcional**
-  **Documentação completa**

---

##  Como Funciona o Driver

### Visão Geral
O driver **AT30TSE752A** permite que aplicações baseadas em Zephyr RTOS realizem a leitura de temperatura através do barramento I2C, abstraindo a complexidade dos registradores do sensor.

### Fluxo de Operação

**1. Inicialização do Sistema:**
```mermaid
graph LR
    App[Aplicação] --> Kernel[Zephyr Kernel]
    Kernel --> Driver[Driver AT30TSE752A]
    Driver --> I2C[Barramento I2C]
    I2C --> Sensor[Sensor Físico]
```

**2. Leitura e Conversão:**
1.  **I2C Read:** O driver lê 2 bytes do registrador `0x00` (Temperature Register).
2.  **Raw Data:** Combina os bytes: `(byte1 << 8) | byte2`.
3.  **Processamento:** Aplica shift e máscara para obter os 12-bits de resolução.
4.  **Conversão:** Calcula `(dados_brutos) × 0.0625` para obter graus Celsius.

### Estruturas de Dados Principais

```c
struct temperature_data {
    int16_t temperature;  // Ex: 0x19A0 convertidos internamente
    bool device_ready;    // Flag de status do sensor
};

struct temperature_config {
    struct i2c_dt_spec i2c;  // Estrutura contendo I2C0 e endereço 0x49
};
```

### Device Tree (Hardware Map)

**Overlay da Placa (`boards/samd21_xplained_pro.overlay`):**
```dts
&i2c0 {
    status = "okay";
    temperature_sensor: at30tse752a@49 {
        compatible = "microchip,at30tse752a"; 
        reg = <0x49>;
        resolution = <12>;
    };
};
```

---

## Objetivos do Projeto

### Objetivo Principal
Desenvolver um driver funcional e robusto para leitura precisa de temperatura utilizando as boas práticas do Zephyr RTOS.

### Objetivos Específicos
- [x] Comunicação I2C estável com sensor AT30TSE752A
- [x] Conversão precisa de dados brutos (Raw) para Celsius
- [x] Integração completa com a API padrão de Sensores do Zephyr
- [x] Portabilidade entre SAM D21 e SAM R21
- [x] Documentação técnica e código limpo

---

## 📅 Cronograma de 8 Semanas

| Semana | Status | Foco | Entregas |
| :---: | :---: | :--- | :--- |
| 1 | ✅ | Ambiente | Setup Zephyr, GitHub Repo |
| 2 | ✅ | Estudo | Análise do Datasheet, Protocolo I2C |
| 3 | ✅ | Estrutura | Criação do Device Tree Binding |
| 4 | ✅ | Comunicação | Implementação de leitura I2C |
| 5 | ✅ | Conversão | Algoritmo de ponto flutuante/inteiro |
| 6 | ✅ | Integração | Sistema completo rodando |
| 7 | ✅ | Testes | Validação em hardware real |
| 8 | ✅ | Documentação | Relatório final e Refatoração |

---

## Implementação Técnica

### Estrutura do Projeto
```bash
projeto-final/
├── 📄 README.md                          # Documentação
├── 🔧 drivers/sensors/
│   └── temperature_at30tse752a.c         # Código fonte do Driver
├── 📋 dts/bindings/sensor/  
│   └── microchip,at30tse752a.yaml        # Binding YAML
├── ⚙️ boards/
│   ├── samd21_xplained_pro.overlay       # Config SAM D21
│   └── samr21_xpro.overlay               # Config SAM R21
├── 📱 src/
│   └── main.c                            # Aplicação de Teste
├── 🔨 prj.conf                           # Configurações do Kernel
└── 🏗️ CMakeLists.txt                     # Build System
```

### Destaques do Código

**Função `sample_fetch` (Captura):**
```c
static int temperature_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    // ...
    // Ler 2 bytes do sensor via I2C Burst Read
    ret = i2c_burst_read_dt(&config->i2c, AT30TSE752A_REG_TEMP, temp_data, 2);
    
    // Combinar bytes: MSB << 8 | LSB
    data->temperature = (temp_data[0] << 8) | temp_data[1];
    return 0;
}
```

**Função `channel_get` (Conversão):**
```c
static int temperature_channel_get(..., struct sensor_value *val) {
    int16_t raw_temp = data->temperature;
    
    // Ajuste para 12-bits e tratamento de sinal
    raw_temp = (raw_temp >> 4);
    if (raw_temp & 0x0800) {
        raw_temp |= 0xF000; // Extensão de sinal para negativos
    }
    
    // Conversão: 0.0625°C por bit
    int32_t temp_micro = raw_temp * 625; 
    val->val1 = temp_micro / 10000;       // Parte inteira
    val->val2 = (temp_micro % 10000) * 100; // Parte decimal (micros)
    
    return 0;
}
```

---

##  Resultados e Validação

### Métricas de Sucesso
| Métrica | Resultado | Status |
| :--- | :--- | :---: |
| **Funcionalidade** | 100% operacional | ✅ |
| **Precisão** | ±0.5°C (conf. datasheet) | ✅ |
| **Estabilidade** | Sem falhas em 1h de teste | ✅ |
| **Portabilidade** | SAM D21 & R21 | ✅ |
| **Tamanho Firmware** | 45.2 KB | ✅ |

### Log de Execução (Hardware Real)
```text
*** Booting Zephyr OS build v3.x.x ***
✅ Sensor AT30TSE752A pronto!
 🌡️ Temperatura: 24.750000°C 🌡️ Temperatura: 24.812500°C 🌡️ Temperatura: 24.875000°C 🌡️ Temperatura: 25.000000°C
```

---

## Como Usar o Driver

### Pré-requisitos
1. Zephyr SDK instalado.
2. Placa SAM D21 ou SAM R21 Xplained Pro.
3. Sensor AT30TSE752A conectado ao barramento I2C (Endereço `0x49` padrão).

### Compilação e Flash

1. **Compilar o projeto:**
   ```bash
   west build -b samd21_xplained_pro projeto-final/
   ```

2. **Gravar na placa:**
   ```bash
   west flash
   ```

3. **Monitorar a saída serial:**
   ```bash
   minicom -D /dev/ttyACM0 -b 115200
   ```

### Exemplo de Uso na Aplicação (`main.c`)

```c
#include <zephyr/drivers/sensor.h>

void main(void) {
    const struct device *sensor = DEVICE_DT_GET(DT_NODELABEL(temperature_sensor));

    if (!device_is_ready(sensor)) {
        return;
    }

    struct sensor_value temp;
    
    while (1) {
        sensor_sample_fetch(sensor);
        sensor_channel_get(sensor, SENSOR_CHAN_AMBIENT_TEMP, &temp);
        
        printk("Temperatura Atual: %d.%06d°C\n", temp.val1, temp.val2);
        k_sleep(K_SECONDS(1));
    }
}
```

---

##  Aprendizados e Desafios

###  Principais Aprendizados
* **Arquitetura Zephyr:** Compreensão profunda sobre como o kernel gerencia drivers e devices.
* **Device Tree:** Importância da descrição de hardware desacoplada do código fonte.
* **Protocolos:** Depuração prática de I2C (timing, pull-ups, endereçamento).

###  Desafios Superados
* **Curva de Aprendizado:** A documentação do Zephyr é extensa; filtrar o que é relevante para drivers customizados foi um desafio inicial.
* **Manipulação de Bits:** Garantir que a conversão dos dados brutos (MSB/LSB) respeitasse o sinal (números negativos) e a resolução de 12 bits.

---

##  Conclusão

O driver desenvolvido atende a todos os requisitos da disciplina de Projeto de Sistemas Embarcados. Ele demonstra não apenas o funcionamento do hardware, mas a capacidade de integração profissional de software embarcado seguindo padrões da indústria.
