# AppViero - Aplicação IoT (Zephyr + OpenThread)

Este diretório contém a implementação da parte de Aplicação (APP) para o Projeto Final de Sistemas Embarcados (UFSM 2025/2).

##  Objetivo
Desenvolver um **Dispositivo Final (End Device)** compatível com a rede Thread que realiza a leitura de sensores ambientais e envia os dados para um Roteador de Borda.

## 🛠 Hardware Utilizado
- **Placa de Desenvolvimento:** Microchip SAM R21 Xplained Pro
- **Módulo de Sensores:** I/O1 Xplained Pro
  - Sensor de Temperatura (AT30TSE75X)
  - Sensor de Luz (Fotossensor)

## 📦 Estrutura do Projeto
- `src/`: Código fonte da aplicação.
- `prj.conf`: Configurações do Kernel e módulos do Zephyr.
- `CMakeLists.txt`: Configurações de compilação.

## 🚀 Como Compilar
A partir da raiz do workspace (`zephyr-app-ufsm00292`):

```bash
west build -p auto -b samr21_xpro AppViero