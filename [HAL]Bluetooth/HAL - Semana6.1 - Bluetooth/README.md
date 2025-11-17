Nesta semana o foco foi:

- Resolver problemas de *link* e Kconfig envolvendo o sample `bluetooth/peripheral_nus`;
- Investigar os erros de `bt_nus_inst_cb_register` / `bt_nus_inst_send` na linkagem;
- Instalar e configurar o **OpenOCD** (xPack) no Windows;
- Tentar usar `west flash` diretamente com OpenOCD;
- Adotar um **plano B confiável** usando o `atprogram` / Microchip Studio para gravar o binário;
- Compilar e testar o sample **`basic/blinky`** para validar que o Zephyr está rodando na SAMD21.

---

## 1. Situação do sample `bluetooth/peripheral_nus`

Com o Zephyr 4.2.x, a API do serviço NUS mudou (funções como `bt_nus_cb_register()` passaram a ser
meros *wrappers* para instâncias internas). Durante a linkagem, apareceram erros como:

- `undefined reference to 'bt_nus_inst_cb_register'`
- `undefined reference to 'bt_nus_inst_send'`

Isso indica que, para usar o NUS na versão atual, é necessário:

- Ajustar o código-fonte (`main.c`) para a nova forma de declarar a instância NUS;
- Ou usar um sample atualizado/documentação oficial de NUS para Zephyr 4.2.x.

Para o trabalho do HAL, ficou decidido **separar** os problemas:
1. Garantir que a pipeline de build + flash funciona (independente do NUS);
2. Depois, se necessário, voltar ao NUS com um exemplo compatível com a versão do Zephyr.

---

## 2. Instalação e configuração do OpenOCD (xPack)

Foi utilizado o **xPack OpenOCD** no Windows, instalado em:

```text
C:\Users\angel\AppData\Roaming\xPacks\xpack-openocd-0.12.0-7\
```

Com isso, o binário principal passou a responder:

```powershell
openocd --version
```

O `west flash` foi então configurado para usar este OpenOCD, por exemplo:

```powershell
cd C:\Users\angel\zephyrproject

py -m west flash -r openocd `
  --openocd "C:\Users\angel\AppData\Roaming\xPacks\xpack-openocd-0.12.0-7\bin\openocd.exe" `
  --openocd-search "C:\Users\angel\AppData\Roaming\xPacks\xpack-openocd-0.12.0-7\share\openocd\scripts"
```

O OpenOCD reconhece corretamente a interface **CMSIS-DAP** da placa:

- Detecta o *device* `at91samd21j18a.cpu`;
- Cria um servidor GDB na porta 3333;
- Mas, durante a fase de acesso ao DP/AP, aparece o erro:

```text
Error: Error connecting DP: cannot read IDR
Error: [at91samd21j18a.cpu] DP initialisation failed
```

Mesmo testando diferentes velocidades (`adapter speed`) e opções de `reset_config`, o erro persiste.
Isso sugere algum detalhe de compatibilidade do EDBG/driver com o OpenOCD em uso.

Por segurança (e para não quebrar o ambiente da universidade), foi adotada uma alternativa.

---

## 3. Plano B: uso do atprogram / Microchip Studio para flash

Foi usado o `atprogram.exe` (ferramenta de linha de comando da Microchip/Atmel) para **apagar** e
**gravar** o microcontrolador. Exemplo de comando de erase (já utilizado com sucesso):

```powershell
& "C:\Program Files (x86)\Atmel\Studio\7.0\atbackend\atprogram.exe" `
  -t edbg -i SWD -d atsamd21j18a erase
```

Depois de gerar o binário com o Zephyr (`zephyr.hex` / `zephyr.elf`) via `west build`, o arquivo foi
gravado na placa via Microchip Studio (GUI) ou via `atprogram`, conforme o fluxo de laboratório.

Esse **plano B** foi fundamental para provar que:

- O binário gerado pelo Zephyr é válido;
- A placa SAMD21 Xplained Pro executa o código normalmente;
- Os problemas com `west flash` + OpenOCD são apenas de ferramenta, não de código.

Mais detalhes de comandos e notas estão em `openocd_notes.txt` (nesta mesma pasta).

---

## 4. Teste de referência: sample `basic/blinky`

Para validar o fluxo, foi usado o sample mais simples possível: `basic/blinky`.

### 4.1. Build

```powershell
cd C:\Users\angel\zephyrproject

py -m west build -b samd21_xpro -p -s zephyr\samples\basic\blinky
```

Saída importante do build:

- Uso de memória dentro dos limites (FLASH ~4.7%, RAM ~12%);
- Geração de `zephyr.elf`, `zephyr.hex`, `zephyr.bin` em `build/zephyr/`.

### 4.2. Flash e observação

Mesmo com o comando `py -m west flash` ainda acusando o erro de OpenOCD ao final, o binário já havia
sido gravado com sucesso via Microchip, e o resultado observado na placa foi:

- **LED do kit piscando continuamente**, confirmando que o Zephyr está rodando no SAMD21.

Para documentação, foi criado um pequeno exemplo de código em C (`blinky_main.c`) nesta pasta,
representando o comportamento do blink usando a API de GPIO do Zephyr.

---

## 5. Arquivos anexos desta semana

Nesta pasta estão:

- `README.md` → Este resumo.
- `blinky_main.c` → Exemplo didático de aplicação Zephyr que pisca o LED `led0` da placa.
- `openocd_notes.txt` → Comandos e observações sobre o uso do OpenOCD + west flash nesta semana.


