
Esta semana foi basicamente focada em:
- Garantir que o ambiente Zephyr no Windows estava 100% funcional;
- Conseguir compilar o *sample* `bluetooth/peripheral_nus` para a placa `samd21_xpro`;
- Entender e ajustar *Devicetree* e `prj.conf` para usar o **SERCOM4** como UART HCI para o BTLC1000;
- Investigar vários erros de *Devicetree* e Kconfig (propriedades `current-speed`, `rxpo`, `txpo`,
  bindings `atmel,sam0-uart`, símbolos `BT_NUS`, `SHELL_BT_NUS`, buffers de ACL, etc.).

---

## 1. Ambiente Zephyr e toolchain

Diretório de trabalho adotado:

```text
C:\Users\angel\zephyrproject
```

Comandos principais usados no PowerShell (sempre dentro de `C:\Users\angel\zephyrproject`):

```powershell
# Build limpo do sample NUS usando a placa samd21_xpro
py -m west build -b samd21_xpro -p -s zephyr\samples\bluetooth\peripheral_nus

# Inspecionar Devicetree gerado
type C:\Users\angel\zephyrproject\build\zephyr\zephyr.dts.pre | findstr /I "sercom4 uart@42001800 bt_hci_uart rxpo txpo"
```

A toolchain ativa é o **Zephyr SDK 0.17.4**, detectado automaticamente pelo CMake.

---

## 2. Objetivo: usar SERCOM4 como UART HCI para o BTLC1000

A ideia é usar o **SERCOM4** da SAMD21 como UART, ligado ao módulo **BTLC1000** da placa Xplained Pro.
Para isso, foi criado um *overlay* de Devicetree específico para o *sample* `peripheral_nus`:

```text
zephyr\samples\bluetooth\peripheral_nus\boards\samd21_xpro.overlay
```

O arquivo correspondente (versão estável desta semana) está junto neste diretório como:

- `samd21_xpro.overlay`

Esse overlay:
- Habilita o `&sercom4` como UART;
- Configura `current-speed` para 115200 baud;
- Ajusta `rxpo` e `txpo` de acordo com os pinos usados na placa;
- Cria o nó `bt_hci_uart` que será usado pelo *stack* Bluetooth do Zephyr como transporte HCI.

O conteúdo completo está logo abaixo na seção de arquivos anexos.

---

## 3. Ajustes em `prj.conf` para Bluetooth + HCI UART

Também foi montado um `prj.conf` específico para esse sample, habilitando:

- BT host;
- Perfil *peripheral*;
- Transporte HCI sobre UART (H:4);
- Alguns ajustes de buffers ACL / evento para acompanhar as mudanças do Zephyr 4.x.

O arquivo está salvo aqui na pasta como:

- `prj.conf`

> **Observação:** esta versão de `prj.conf` é **didática**, pensada para documentar os testes.
> Em Zephyr 4.2.x alguns símbolos mudaram (por exemplo, a API do NUS e opções de fluxo H4),
> então o arquivo pode precisar de ajustes finos se for reaproveitado como base de build.

---

## 4. Erros encontrados e lições

Principais problemas vistos nesta fase:

1. **Devicetree / bindings:**
   - Mensagens do tipo: `devicetree error: 'current-speed' appears in ... but is not declared in 'properties:'`.
   - Erros de `rxpo`/`txpo` obrigatórios para o binding `atmel,sam0-uart`.

   → Isso mostrou que, para o SERCOM4 virar UART, é obrigatório declarar `current-speed`, `rxpo` e `txpo`
   no nó certo (`sercom@42001800`), em conformidade com `atmel,sam0-uart.yaml`.

2. **Kconfig / símbolos indefinidos:**
   - `BT_NUS` e `SHELL_BT_NUS` dependeram de opções que mudaram entre versões do Zephyr.
   - Símbolos legados como `BT_H4_ENABLE_FLOW_CONTROL` e `BT_HCI_UART_BAUDRATE` passaram a ser
     desconhecidos no Zephyr 4.2.x.

   → A solução parcial nesta semana foi **comentar** essas opções “legadas” e focar primeiro em fazer
   o HCI UART funcionar, deixando o NUS/Shell para uma etapa posterior, já com a documentação da nova API.

3. **Buffers de ACL vs eventos:**
   - O cabeçalho `bluetooth/buf.h` passou a ter um `BUILD_ASSERT` exigindo que:
     `CONFIG_BT_BUF_EVT_RX_COUNT > CONFIG_BT_BUF_ACL_TX_COUNT`.

   → É necessário garantir essa desigualdade no `prj.conf` para evitar erro de build.

---

## 5. Arquivos anexos desta semana


- `README.md` → Este resumo.
- `prj.conf` → Configuração de Bluetooth + HCI UART usada como base.
- `samd21_xpro.overlay` → Overlay de Devicetree ativando o SERCOM4 como UART HCI para o BTLC1000.
