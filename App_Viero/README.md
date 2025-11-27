O trabalho foi realizado utilizando os dados da pasta samr21_thread_sensor em coletivo com alguns colegas da disciplina.

Relatório de Impedimento Técnico: Limitação de Hardware na Integração Zephyr + OpenThread
1. Contexto e Status do Projeto O objetivo central deste trabalho era desenvolver uma aplicação de IoT utilizando o sistema operacional de tempo real Zephyr RTOS integrado à pilha de rede OpenThread, operando na plataforma de hardware Microchip SAM R21 Xplained Pro.

A etapa de Aplicação (APP) foi concluída com sucesso. Foi desenvolvido um firmware baseado no Zephyr capaz de inicializar os drivers I2C e realizar a leitura precisa dos sensores de temperatura (AT30TSE75X) e luminosidade disponíveis no módulo de extensão I/O1. O sistema operacional gerencia corretamente as threads de leitura e os drivers de periféricos.

2. O Obstáculo Técnico: Estouro de Memória SRAM Durante a fase de integração da pilha de rede OpenThread ao projeto funcional dos sensores, o processo de linking (ligação) do firmware falhou consistentemente, reportando um erro crítico de Estouro de Memória RAM (RAM Overflow).

O microcontrolador ATSAMR21G18A, presente na placa utilizada, possui uma limitação física de 32 KB de SRAM. Esta quantidade de memória provou-se insuficiente para acomodar simultaneamente:

O Kernel do Zephyr: Mesmo em configurações mínimas, o escalonador, os objetos de sincronização e os drivers de hardware consomem uma parcela significativa da memória.

A Pilha OpenThread: O protocolo Thread é robusto e complexo, exigindo buffers substanciais para tabelas de roteamento, buffers de pacotes IPv6/6LoWPAN e gerenciamento de segurança (criptografia).

A Aplicação de Usuário: As variáveis e buffers necessários para a leitura dos sensores.

3. Medidas de Mitigação e Otimização Diversas técnicas de engenharia de software foram aplicadas na tentativa de contornar a limitação de hardware:

Minimização do Kernel: Desativação de subsistemas não essenciais, como Logging (printk), Shell de depuração e suporte a ponto flutuante não utilizado.

Configuração MTD (Minimal Thread Device): Tentativa de compilar o dispositivo apenas como um "End Device" (sem capacidades de roteamento), o que teoricamente reduz o consumo de memória.

Redução de Buffers: Diminuição agressiva do tamanho das pilhas (stack sizes) das threads e da quantidade de buffers de rede (CONFIG_NET_PKT_*).

Apesar desses esforços ("capar o projeto"), o consumo estático de RAM permaneceu acima dos 32 KB disponíveis, tornando a compilação inviável.

4. Análise de Alternativas: Bare Metal vs. RTOS Uma solução técnica viável para este hardware seria a implementação em Bare Metal (programação direta no hardware, sem sistema operacional), ou utilizando o framework proprietário da Microchip (ex: através do Advanced Software Framework ou MPLAB Harmony). Isso eliminaria o overhead do Zephyr OS, liberando RAM suficiente para a pilha de rede.

No entanto, esta alternativa foi descartada deliberadamente. O requisito pedagógico fundamental da disciplina e do projeto (UFSM00292) especifica explicitamente o uso do Zephyr RTOS. Migrar para uma solução Bare Metal resolveria o problema técnico imediato, mas desviaria do propósito acadêmico de aprendizado e utilização de Sistemas Operacionais de Tempo Real modernos.

5. Conclusão Conclui-se que o hardware SAM R21/D21, com seus 32 KB de RAM, apresenta uma limitação física impeditiva para a execução da stack completa do Zephyr OS combinada com o OpenThread na versão atual das bibliotecas. O projeto entrega, portanto, a camada de aplicação e drivers funcionais sobre o Zephyr, demonstrando o domínio do RTOS e dos periféricos, mas isolada da malha de rede devido às restrições de hardware documentadas.