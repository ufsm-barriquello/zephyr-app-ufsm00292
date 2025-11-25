# SAMR21 + IO1 Sensor Node  
### IEEE 802.15.4 (conceito) + USB → MQTT (implementação real)  
### Trabalho Final de Sistemas Embarcados – Gabriel Schuster – UFSM

---

## 📌 Visão Geral

Este projeto implementa um **nó sensor** utilizando:

- **SAMD21 + RF233 (SAMR21 Xplained Pro)**
- **IO1 Xplained Pro (sensores de temperatura e luz)**
- **Comunicação local via USB → MQTT**
- **802.15.4 + UDP implementado (mas não utilizado na prática)**

O objetivo é capturar dados ambientais e enviá‑los para um **dashboard no PC**, além de deixar documentado o uso de **rede 802.15.4** mesmo sem o hardware receptor.

---

## 🧩 Arquitetura Geral

### ✔ Funciona hoje
- SAMR21 lê sensores (I2C + ADC)
- Envia JSON via USB CDC
- Python lê JSON → envia ao Mosquitto MQTT
- Dashboard lê e plota em tempo real

### ✔ Implementado mas não usado
- 802.15.4 com IPv6 + UDP
- Configuração de rede Thread/OpenThread (3.7 e 4.2)

### ❌ Não utilizado devido ao hardware
- Sem dongle IEEE 802.15.4 compatível com Linux  
  Portanto, o pacote 802.15.4 não sai “pelo ar”.

Tudo isso está completamente documentado no relatório final.

---

## 📡 Sensores Utilizados

### Temperatura – **AT30TSE75x**
- Interface: **I2C**
- Endereço: `0x4F`

### Luz – **TEMT6000**
- Interface: **ADC**
- Canal: **AIN6 / PA06**
- Resolução: **12 bits**

---

## 🛠️ Arquivos do Projeto

| Arquivo | Descrição |
|--------|-----------|
| `src/main.c` | Firmware Zephyr com sensores + MQTT-over-USB + 802.15.4 |
| `prj.conf` | Configurações (rede, ADC, I2C, USB, logs) |
| `samr21_xpro.overlay` | Mapeamento ADC para PA06 |
| `samr21_mqtt.py` | Script PC → lê USB → publica no MQTT |
| `mqtt_plot.py` | Dashboard gráfico com CSV |
| `leituras.csv` | Arquivo gerado automaticamente pelo dashboard |

---

## 🔌 Firmware – Compilação

```bash
west build -b samr21_xpro --pristine
west flash
```

---

## 🖥️ No PC (WSL)

### 1️⃣ Instalar Mosquitto

```bash
sudo apt install mosquitto mosquitto-clients
sudo service mosquitto start
```

### 2️⃣ Rodar o script USB → MQTT

```bash
python3 samr21_mqtt.py
```

### 3️⃣ Escutar mensagens

```bash
mosquitto_sub -h localhost -t "samr21/sensors" -v
```

### 4️⃣ Abrir Dashboard

```bash
python3 mqtt_plot.py
```

---

## 🔍 Exemplo de JSON recebido

```json
{
  "src": "samr21",
  "temp": 29.50,
  "light": 75.9
}
```

---

## 🚨 Sobre o 802.15.4 – Importante

- A camada IEEE 802.15.4 foi **totalmente configurada**.
- IPv6 link-local `fe80::1234` configurado.
- Socket UDP implementado.
- Porém o envio falha com `sendto = -1`.

📌 Motivo real:  
**O PC precisa de uma interface IEEE 802.15.4 (nl802154).**  
Sem isso, o pacote nunca chega ao destino.

✨ O relatório inclui:
- Logs completos
- Prints dos erros
- Trechos de código de tentativas
- Explicação técnica do motivo

---

## 📝 Como o APP foi finalizado

- O código 802.15.4 foi mantido e documentado.
- A transmissão real é feita por USB → MQTT.
- O dashboard lê em tempo real e gera gráficos.
- Todos os requisitos funcionais foram cumpridos.

---

## 📚 Autor

**Gabriel Fronza Schuster**  
Engenharia de Computação – UFSM  
2025

---

## ✔ Status do Projeto

| Recurso | Status |
|--------|--------|
| I2C Temperatura | ✔ Funcionando |
| ADC Luz | ✔ Funcionando |
| MQTT USB | ✔ Funcionando |
| Dashboard | ✔ Funcionando |
| CSV automático | ✔ Funcionando |
| 802.15.4 stack | ✔ Implementado |
| 802.15.4 transmissão real | ❌ Sem dongle |

---

## 📦 Licença

MIT – 2025
