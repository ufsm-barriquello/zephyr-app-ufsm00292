Objetivo

Integrar o **SAMD21 Xplained Pro** (`samd21_xpro`) rodando **Zephyr** com o módulo
**BTLC1000 Xplained Pro** atuando como **controlador BLE externo via HCI-UART**, e
rodar o exemplo `samples/bluetooth/peripheral_hr` (Heart Rate Peripheral).


## Hardware utilizado

- Placa **SAMD21 Xplained Pro**
- Módulo **BTLC1000 Xplained Pro** (no conector EXT1, com UART em PB8/PB9)
- Cabo USB da placa SAMD21 para o PC (alimentação + programação + console)



- `samd21_xpro.conf`  ➜  
  `C:\Users\angel\zephyrproject\zephyr\samples\bluetooth\peripheral_hr\boards\samd21_xpro.conf`

- `samd21_xpro.overlay` ➜  
  `C:\Users\angel\zephyrproject\zephyr\samples\bluetooth\peripheral_hr\boards\samd21_xpro.overlay`


---

## Arquivos deste diretório

### 1. `samd21_xpro.conf`

Configurações extras de Kconfig para a board `samd21_xpro` neste sample:

- Habilita uso de **controlador BLE externo via HCI UART (H:4)**.
- Mantém o Zephyr só como **Host** (sem controlador interno).

```ini
# Habilita uso de controlador BLE externo (BTLC1000) via HCI UART
CONFIG_BT_HCI=y
CONFIG_BT_H4=y

# Logs de debug do Bluetooth (opcional, mas ótimo pra ver o HCI subindo)
CONFIG_BT_DEBUG_LOG=y
CONFIG_BT_LOG_LEVEL_DBG=y
```

> O resto da configuração BT (peripheral, serviços, etc.) já vem do `prj.conf` do próprio
> exemplo `peripheral_hr`.

---

### 2. `samd21_xpro.overlay`

Arquivo Devicetree overlay para:

- Configurar **SERCOM4** como UART em modo **4-wire (RTS/CTS)** a 1 Mbps,
- Mapear os pinos PB8/PB9/PB10/PB11 para o BTLC1000,
- Informar ao Zephyr que o controlador BT está no nó `bt_hci_uart`.

> **Atenção:** dependendo da versão do Zephyr/SDK, pode ser necessário ajustar os valores de `pinmux`
> para bater exatamente com os defines internos da família SAMD21. Este arquivo registra o estado
> atual dos testes e pode precisar de ajustes finos depois.

---

## Como compilar e gravar

No **Windows PowerShell**:

```powershell
cd C:\Users\angel\zephyrproject\zephyr

py -m west build -b samd21_xpro samples\bluetooth\peripheral_hr -p

py -m west flash
```

Se tudo der certo, o console deve mostrar o Zephyr iniciando e o Bluetooth inicializado
usando HCI UART.

---

## Testando com o celular

1. Conecte o SAMD21 Xplained Pro via USB no PC.
2. Abra um terminal serial (115200 8N1) na porta COM da placa (EDBG).
3. Aperte o botão de reset da SAMD21 e veja os logs do Zephyr.
4. No celular, abra um app de scanner BLE (nRF Connect, LightBlue, etc.).
5. Procure por um dispositivo chamado algo como **"Zephyr HRM"**.
6. Conecte e confira se aparece o serviço de **Heart Rate**.

Se aparecer e conseguir ler as características, significa que:

- O SAMD21 está rodando o host Bluetooth do Zephyr,
- O BTLC1000 está funcionando como controlador HCI via UART.

---

## Histórico / Notas de debug

- **Erro anterior**:  
  ```text
  devicetree error: samd21_xpro.overlay:4 (column 23): parse error: expected number or parenthesized expression
  ```
  Isso ocorreu porque o `pinmux` usava símbolos que não existiam ou não estavam definidos
  no contexto daquele Devicetree.  
  Este overlay foi ajustado, mas ainda pode precisar de pequenos acertos conforme a versão
  específica do Zephyr/SDK usada.
