# PortaSerial

Biblioteca C++ para o Raspberry Pi Pico W que encapsula o uso da UART e padroniza o envio e a recepção de dados seriais. A classe `PortaSerial` centraliza a inicialização da UART, o gerenciamento de buffers e as operações de leitura e escrita, favorecendo aplicações modulares com o Pico SDK.

## Recursos principais

- Inicialização simples e explícita de qualquer interface `uart_inst_t` do RP2040.
- Envio de caracteres, textos ou valores numéricos com conversão segura em buffer estático.
- Leitura de dados com timeout configurado na biblioteca para evitar bloqueios prolongados.
- Funções utilitárias para consulta de disponibilidade de dados, verificação de possibilidade de transmissão e limpeza de buffer.
- Método de reinicialização para restaurar rapidamente a UART em cenários de falha.

## Requisitos

- Raspberry Pi Pico W (RP2040) com SDK 2.2.0.
- Ferramentas de build configuradas com CMake e Ninja (estrutura padrão do Pico SDK).
- Bauds, pinos e UART disponíveis conforme o hardware do seu projeto. O exemplo deste repositório utiliza:
  - UART1 em 115200 baud.
  - TX no GP8 e RX no GP9.
  - Conversão de CR/LF realizada pelo backend UART (`stdio_set_translate_crlf`).

## Como adicionar ao seu projeto

1. Garanta que a pasta `PortaSerial` esteja dentro do diretório raiz do seu projeto (ao lado do `CMakeLists.txt` principal).
2. No `CMakeLists.txt` principal, inclua a subpasta e vincule a biblioteca ao seu executável:

```cmake
add_subdirectory(PortaSerial/src)

add_executable(seu_programa
    main.cpp
    # outros fontes
)

target_link_libraries(seu_programa
    PRIVATE
        PortaSerial
)
```

3. Lembre-se de chamar `pico_enable_stdio_uart` ou manter o backend de logs configurado quando necessário.

## Exemplo de uso rápido

```cpp
#include "pico/stdlib.h"
#include "PortaSerial.h"

static uart_inst_t * const INTERFACE_UART = uart1;
constexpr uint32_t TAXA_BAUD_PADRAO = 115200U;
constexpr uint GPIO_TX = 8U;
constexpr uint GPIO_RX = 9U;

int main() {

    PortaSerial porta_serial(INTERFACE_UART, TAXA_BAUD_PADRAO, GPIO_TX, GPIO_RX);

    if (!porta_serial.iniciar()) {
        while (true) {
            tight_loop_contents();
        }
    }

    porta_serial.enviarTextoComNovaLinha("Serial pronta para uso");

    while (true) {
        if (porta_serial.haDadosDisponiveis()) {
            char caractere_recebido = '\0';

            if (porta_serial.lerCaractere(caractere_recebido)) {
                porta_serial.enviarCaractere(caractere_recebido);
            }
        }

        sleep_ms(10);
    }
}
```

### Pontos de atenção no exemplo

- `TAXA_BAUD_PADRAO`, `GPIO_TX` e `GPIO_RX` são constantes `constexpr` para evitar números mágicos.

## API e exemplos da classe `PortaSerial`

Cada item combina uma breve descrição funcional e um exemplo imediato.

Inicializa a UART com os parâmetros definidos.

```cpp
PortaSerial porta_serial(uart1, 115200, 8, 9);

porta_serial.iniciar();
porta_serial.enviarTextoComNovaLinha(TEXTO_PRONTO);
```

Transmite um único caractere aguardando espaço no hardware.

```cpp
PortaSerial porta_serial(uart1, 115200, 8, 9);

porta_serial.iniciar();
porta_serial.enviarCaractere('A');
```

Envia uma cadeia de caracteres terminada em `\0`.

```cpp
PortaSerial porta_serial(uart1, 115200, 8, 9);

porta_serial.iniciar();
porta_serial.enviarTexto("Ping");
```

Adiciona `"\r\n"` após o texto enviado.

```cpp
PortaSerial porta_serial(uart1, 115200, 8, 9);

porta_serial.iniciar();
porta_serial.enviarTextoComNovaLinha("Pronto");
```

Converte um valor numérico para texto em buffer interno e envia pela UART.

```cpp
PortaSerial porta_serial(uart1, 115200, 8, 9);

porta_serial.iniciar();
porta_serial.enviarValor(23.5F);
```

Lê um caractere quando houver dados disponíveis, retornando falso em caso de timeout.

```cpp
PortaSerial porta_serial(uart1, 115200, 8, 9);

porta_serial.iniciar();

char caractere_recebido = '\0';

porta_serial.lerCaractere(caractere_recebido);
```

Coleta caracteres até atingir o delimitador informado ou o limite do buffer.

```cpp
PortaSerial porta_serial(uart1, 115200, 8, 9);

porta_serial.iniciar();

char buffer_texto[32] = {0};

porta_serial.lerTexto(buffer_texto, sizeof(buffer_texto));
```

Indica se há bytes prontos para leitura imediata.

```cpp
PortaSerial porta_serial(uart1, 115200, 8, 9);

porta_serial.iniciar();

bool possui_dados = porta_serial.haDadosDisponiveis();
(void)possui_dados;
```

Informa se a UART pode aceitar novos dados sem bloqueio.

```cpp
PortaSerial porta_serial(uart1, 115200, 8, 9);

porta_serial.iniciar();

if (porta_serial.podeTransmitir()) {
    porta_serial.enviarTexto("OK");
}
```

Remove bytes residuais da fila de recepção.

```cpp
PortaSerial porta_serial(uart1, 115200, 8, 9);

porta_serial.iniciar();
porta_serial.limparBuffer();
```

Desliga e liga novamente a UART reaproveitando os parâmetros atuais.

```cpp
PortaSerial porta_serial(uart1, 115200, 8, 9);

porta_serial.iniciar();
porta_serial.reiniciar();
```

## Boas práticas

- Evite alocação dinâmica nos buffers de recepção. O método `lerTexto` trabalha diretamente com buffers externos fornecidos por você.
- Prefira usar `enviarValor` para logar números em vez de montar strings temporárias manualmente.
- Utilize `podeTransmitir` antes de enviar grandes blocos caso sua aplicação tenha requisitos de tempo críticos.
- Em fluxos interativos, chame `limparBuffer` antes de processar novos comandos para evitar dados residuais.