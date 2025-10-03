# Console MineBash para Cartão SD no Raspberry Pi Pico W

## Visão geral
O projeto `main.cpp` integra a biblioteca `CartaoSD`, a camada de E/S serial `PortaSerial` e o console interativo `MineBash` para expor, via UART, um shell simples de gerenciamento de arquivos em um cartão SD conectado ao Raspberry Pi Pico W. A aplicação inicia o barramento SPI0, monta o sistema de arquivos FAT (FatFs 0.15) e oferece comandos para listar, criar, apagar, formatar e visualizar dados da mídia.

## Hardware utilizado
- **Placa:** Raspberry Pi Pico W (RP2040)
- **Cartão SD:** conectado ao barramento SPI0
  - MISO → GP16
  - MOSI → GP19
  - SCK → GP18
  - CS → GP17
- **Interface serial para logs e console:** UART1 a 115200 bps
  - TX → GP8
  - RX → GP9
- **Alimentação:** USB 5 V do computador

> Ajuste os GPIOs no construtor de `CartaoSD` e `PortaSerial` caso utilize outra pinagem.

## Dependências de software
- Pico SDK 2.2.0 (importado via `pico_sdk_import.cmake`)
- Toolchain arm-none-eabi compatível (GNU Arm Embedded 14.2 ou superior recomendado)
- CMake ≥ 3.13 e Ninja (ou Make)
- Bibliotecas do projeto:
  - `CartaoSD` (camada SPI + FatFs)
  - `PortaSerial` (UART orientada a objeto)
  - `pico_stdlib` e `pico_stdio_uart` fornecidas pelo SDK

As fontes do FatFs estão incluídas em `CartaoSD/src/ff15`.

## Estrutura principal
```
MineBash/
├── CMakeLists.txt          # Projeto principal
├── main.cpp                # Ponto de entrada: inicializa SD, UART e MineBash
├── src/
│   ├── mineBash.cpp/.h     # Shell serial para o cartão SD
├── CartaoSD/               # Biblioteca de abstração do cartão SD (FatFs + SPI)
└── PortaSerial/            # Biblioteca para comunicação UART
```
## Fluxo de uso do MineBash
- `listar [caminho]` — exibe arquivos e pastas.
- `criar_pasta <caminho>` — cria diretórios.
- `criar_arquivo <caminho>` — gera arquivos vazios.
- `exibir_arquivo <caminho>` — mostra o conteúdo no terminal.
- `escrever_arquivo [-n] <caminho> "texto"` — acrescenta dados.
- `apagar_pasta [-r] <caminho>` ou `apagar_arquivo <caminho>` — remove entradas.
- `formatar` — recria o sistema de arquivos na unidade `0:` (usa buffer de trabalho interno).

Cada comando é encaminhado pela UART e processado pelo objeto `MineBash`, que utiliza a API de alto nível exposta por `CartaoSD`.