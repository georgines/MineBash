#include "PortaSerial.h"

#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"

static constexpr uint32_t ESPERA_CURTA_US = 10;
static constexpr uint32_t AGUARDO_CARACTERE_TEXTO_US = 1000U;
static constexpr uint32_t TEMPO_MAXIMO_SEM_DADOS_US = 20000U;

PortaSerial::PortaSerial(uart_inst_t* uart_escolhida,
                         uint32_t taxa_baud,
                         uint pino_tx,
                         uint pino_rx)
    : uartEscolhida(uart_escolhida),
      taxaBaud(taxa_baud),
      pinoTx(pino_tx),
      pinoRx(pino_rx) {
}

bool PortaSerial::iniciar() {
    if (uartEscolhida == nullptr) {
        return false;
    }

    stdio_uart_deinit();    
    stdio_uart_init_full(uartEscolhida, taxaBaud, pinoTx, pinoRx);
    if (taxaBaud == 0U) {
        return false;
    }

    return true;
}

bool PortaSerial::enviarCaractere(char caractere) {
    if (uartEscolhida == nullptr) {
        return false;
    }

    while (!uart_is_writable(uartEscolhida)) {
        sleep_us(ESPERA_CURTA_US);
    }

    uart_putc_raw(uartEscolhida, caractere);

    return true;
}

bool PortaSerial::enviarTexto(const char* texto) {
    if (texto == nullptr) {
        return false;
    }

    const char* ponteiro_atual = texto;

    while (*ponteiro_atual != '\0') {
        bool envio_ok = enviarCaractere(*ponteiro_atual);

        if (!envio_ok) {
            return false;
        }

        ponteiro_atual++;
    }

    return true;
}

bool PortaSerial::enviarTextoComNovaLinha(const char* texto) {
    if (!enviarTexto(texto)) {
        return false;
    }

    return enviarTexto(FIM_LINHA);
}

bool PortaSerial::lerCaractere(char& caractere) {
    if (!haDadosDisponiveis()) {
        return false;
    }

    caractere = static_cast<char>(uart_getc(uartEscolhida));

    return true;
}

size_t PortaSerial::lerTexto(char* destino, size_t tamanho_maximo, char delimitador) {
    if (destino == nullptr) {
        return 0U;
    }

    if (tamanho_maximo == 0U) {
        return 0U;
    }

    if (uartEscolhida == nullptr) {
        return 0U;
    }

    size_t indice_atual = 0U;
    uint32_t tempo_sem_dados_us = 0U;

    while (indice_atual < (tamanho_maximo - 1U)) {
        bool dados_disponiveis = uart_is_readable_within_us(uartEscolhida,
                                                            AGUARDO_CARACTERE_TEXTO_US);

        if (!dados_disponiveis) {
            tempo_sem_dados_us += AGUARDO_CARACTERE_TEXTO_US;

            if (tempo_sem_dados_us >= TEMPO_MAXIMO_SEM_DADOS_US) {
                if (indice_atual == 0U) {
                    return 0U;
                }

                break;
            }

            continue;
        }

        tempo_sem_dados_us = 0U;

        char caractere_lido = static_cast<char>(uart_getc(uartEscolhida));

        if (caractere_lido == delimitador) {
            break;
        }

        if (caractere_lido == '\r') {
            continue;
        }

        destino[indice_atual] = caractere_lido;
        indice_atual++;
    }

    destino[indice_atual] = '\0';

    return indice_atual;
}

bool PortaSerial::haDadosDisponiveis() {
    if (uartEscolhida == nullptr) {
        return false;
    }

    return uart_is_readable(uartEscolhida);
}

bool PortaSerial::podeTransmitir() {
    if (uartEscolhida == nullptr) {
        return false;
    }

    return uart_is_writable(uartEscolhida);
}

void PortaSerial::limparBuffer() {
    if (uartEscolhida == nullptr) {
        return;
    }

    while (haDadosDisponiveis()) {
        char caractere_descartado = '\0';
        bool leitura_ok = lerCaractere(caractere_descartado);

        if (!leitura_ok) {
            break;
        }
    }
}

bool PortaSerial::reiniciar() {
    if (uartEscolhida == nullptr) {
        return false;
    }

    uart_deinit(uartEscolhida);

    return iniciar();
}
