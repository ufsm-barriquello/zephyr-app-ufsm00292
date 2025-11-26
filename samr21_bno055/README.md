# Projeto Final — UFSM00292 – Projeto de Sistemas Embarcados
## Sistema IoT com Zephyr RTOS, OpenThread e IMU BNO055 no SAMR21 Xplained Pro

Este repositório contém o desenvolvimento completo do projeto final da disciplina **UFSM00292 – Projeto de Sistemas Embarcados**, ministrada pelo Prof. Carlos Henrique Barriquello (UFSM – Departamento de Eletrônica e Computação).

---

# 📡 Resumo da Aplicação Desenvolvida

Este projeto implementa um **nó sensor IoT com IMU (BNO055) rodando no SAMR21 Xplained Pro**, integrado ao **Zephyr RTOS**, com suporte:

- Comunicação USB-CDC  
- I²C utilizando SERCOM1 nativo do SAMR21  
- Leitura de dados do BNO055 (quaternion + ângulos de Euler)  
- Formato JSON enviado pela interface USB  
- Visualizador em **Python + Matplotlib 3D**, incluindo  
  - Cubo 3D  
  - Avião 3D  
  - Drone quadricóptero 3D animado

O visualizador gira em tempo real conforme a placa SAMR21 é movimentada.

---

# 🧩 Organização do Projeto (HAL + APP)

## HAL – Camada de Hardware
- Portabilidade para SAMR21 Xplained Pro  
- Habilitação do barramento I²C (SERCOM1 → EXT1/EXT3)  
- Configuração do USB-CDC  
- Desenvolvimento do driver de comunicação com BNO055  
- Testes de conexão e identificação (chip ID 0xA0)  
- Loop de aquisição em 20 Hz  

## APP – Aplicação IoT
- Formatação dos dados sensorias em JSON  
- Visualização 3D em Python  
- Arquitetura compatível com OpenThread  

---

# 🛰️ Hardware Utilizado

- SAMR21 Xplained Pro  
- BNO055 Xplained Pro  
- Cabo USB  
- PC com Python 3.x  

---

# 🧠 Estrutura do Repositório

```
/samr21_bno055
    ├── src/main.c
    ├── prj.conf
    ├── CMakeLists.txt
    ├── README.md

/python_visualizer
    ├── viewer_cube.py
    ├── viewer_drone.py
```

---

# 🔧 Como Compilar (Zephyr + west)

```bash
west build -b samr21_xpro --pristine
west flash
```

---

# 🔌 Como Executar o Visualizador em Python

```bash
pip install numpy pyserial matplotlib
python viewer_drone.py
```

---

# 📊 Resultados Obtidos

- Leitura estável da IMU  
- Comunicação USB consistente  
- Visualização 3D fluida  
- Portabilidade completa para SAMR21  


# 🙋‍♂️ Autor

Gabriel Fronza Schuster  
UFSM — Eletrônica e Computação  
Projeto Final – UFSM00292  
