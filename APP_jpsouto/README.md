**João Pedro Souto Fernandes**  
Disciplina: Projeto de Sistemas Embarcados
Professor: Carlos Henrique Barriquello  
Período: 2025/2  

---

# Comunicação entre duas placas SAM R21 Xplained Pro usando Zephyr RTOS

## Objetivo
Estabelecer uma **comunicação sem fio ponto a ponto** entre duas placas **SAM R21 Xplained Pro**, utilizando o **rádio IEEE 802.15.4** embutido no microcontrolador **ATSAMR21**.  
O objetivo inicial é enviar e receber mensagens simples entre as placas, validando o funcionamento do módulo de rádio sob o **Zephyr RTOS**.

---

##  Ambiente de Desenvolvimento

| Item | Descrição |
|:--|:--|
| **Sistema Operacional** | Ubuntu via WSL2 (Windows 10/11) |
| **SDK Zephyr** | 0.16.8 |
| **Board** | `atsamr21_xpro` |
| **Compilador** | `arm-zephyr-eabi-gcc` |
| **Gerenciador de Projeto** | `west` |

---


## Tipo de Comunicação

- **Protocolo:** IEEE 802.15.4 (2.4 GHz)
- **Modo:** Peer-to-Peer
- **Direção:** Unidirecional (TX/RX)

---

## Configuração Base (`prj.conf`)

As opções de configuração essenciais para ativar o rádio e a pilha de rede 802.15.4 serão definidas no arquivo `prj.conf`, incluindo:

- Ativação do subsistema de rede (`CONFIG_NETWORKING`)
- Suporte ao protocolo IEEE 802.15.4 (`CONFIG_IEEE802154`)
- Configurações de buffer e log de rede (`CONFIG_NET_BUF_*`, `CONFIG_NET_LOG`)

Essas definições permitem o envio e recebimento de pacotes via rádio.

---

## Etapas Gerais

1. **Configuração do ambiente Zephyr**
   - Instalar o SDK, ferramentas e dependências.
   - Inicializar o workspace com `west init` e `west update`.

2. **Criação do diretório do aluno**
   - Criar uma pasta com o nome pessoal dentro do projeto principal.
   - Adicionar o código-fonte e arquivos de configuração.

3. **Implementação**
   - Escrever o código do transmissor (`main_tx.c`).
   - Escrever o código do receptor (`main_rx.c`).

4. **Compilação e gravação**
   - Compilar com o comando `west build -b atsamr21_xpro`.
   - Gravar com `west flash`.

5. **Teste de comunicação**
   - Conectar as duas placas via USB.
   - Monitorar a saída serial em ambas as placas.
   - Verificar se as mensagens enviadas são recebidas corretamente.

---

## Resultados Esperados

- Comunicação ponto a ponto bem-sucedida entre duas placas SAM R21.  
- Envio e recepção de mensagens com logs no terminal serial.  
- Alcance funcional em ambiente aberto (~10 m).  
- Estabilidade na taxa de transmissão (250 kbps nominal).

---