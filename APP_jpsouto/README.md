# README

## Descrição

Este projeto implementa um nó IoT utilizando Zephyr RTOS na placa SAMR21 Xplained Pro, integrado ao sensor BNO055 via I²C. O sistema coleta dados de orientação, formata em JSON e envia ao PC via USB-CDC, onde uma aplicação Python realiza visualização 3D em tempo real.  
A arquitetura segue o modelo exigido na disciplina, com separação entre HAL e APP, e está preparada para futura inclusão do OpenThread.

---

## Recursos Implementados

- Execução do Zephyr RTOS no SAMR21  
- Comunicação I²C com o BNO055  
- Modo NDOF para fusão sensorial  
- Envio periódico de dados via USB-CDC  
- Visualização 3D em Python  
- Estrutura modular dividida em HAL e APP

---

## Arquitetura

**HAL:**  
- Configuração e comunicação I²C  
- Inicialização do BNO055 e leitura dos registros  
- Seleção do modo NDOF  
- Comunicação USB-CDC  

**APP:**  
- Threads cooperativas para leitura e transmissão  
- Formatação dos dados em JSON  
- Gerenciamento do fluxo MCU → PC

---

## Fundamentação Técnica

**Zephyr RTOS**  
- Kernel leve, multitarefa, modular  
- Drivers nativos para USB, I²C e sensores  
- Configuração via device tree

**BNO055**  
- IMU de 9DoF com fusão interna  
- Entrega Euler e quaternions sem necessidade de filtros adicionais

**OpenThread**  
- Requer 14–18 KB de RAM  
- Não foi habilitado devido à limitação da SAMR21 (32 KB totais)

---

## Aplicação Python

A aplicação Python recebe os dados formatados em JSON e atualiza modelos 3D por meio do VPython.  
Serve como ferramenta de validação visual do comportamento do sensor e da comunicação com o firmware.

---

## Resultados

- Leitura estável do BNO055  
- Taxa de atualização de 50 ms mantida  
- Visualização 3D responsiva e coerente com o movimento real  
- Comunicação MCU → PC validada integralmente

---

## Desafios

- Limitação de RAM que impede a habilitação do OpenThread  
- Ajuste das pilhas das threads devido a stack overflow  
- Tempo de inicialização necessário para leitura correta do Chip ID  
- Configuração do I²C no device tree

---

## Trabalhos Futuros

- Utilização de MCU com maior RAM para suportar Thread  
- Substituição da comunicação USB por 802.15.4  
- Suporte a múltiplos sensores  
- Expansão da aplicação Python para registro e análise de dados
