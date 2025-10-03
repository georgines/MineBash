#ifndef PORTASERIAL_H
#define PORTASERIAL_H

#include <cstdint>
#include <cstddef>
#include <cstdio>

#include "hardware/uart.h"
#include "pico/types.h"

class PortaSerial {
public:
    static constexpr size_t TAMANHO_BUFFER_VALOR = 32U;
    inline static constexpr const char FIM_LINHA[] = "\r\n";

    PortaSerial(uart_inst_t* uartEscolhida, uint32_t taxaBaud, uint pinoTx, uint pinoRx);

    bool iniciar();

    bool enviarCaractere(char caractere);

    bool enviarTexto(const char* texto);

    bool enviarTextoComNovaLinha(const char* texto);

    template <typename TipoValor>
    bool enviarValor(const TipoValor& valor) {
        char buffer[TAMANHO_BUFFER_VALOR] = {0};
        int quantidade_formatada = std::snprintf(buffer,
                                                 TAMANHO_BUFFER_VALOR,
                                                 "%g",
                                                 static_cast<double>(valor));

        if (quantidade_formatada <= 0) {
            return false;
        }

        return enviarTexto(buffer);
    }

    bool lerCaractere(char& caractere);

    size_t lerTexto(char* destino, size_t tamanho_maximo, char delimitador = '\n');

    bool haDadosDisponiveis();

    bool podeTransmitir();

    void limparBuffer();

    bool reiniciar();

private:
    uart_inst_t* uartEscolhida;
    uint32_t taxaBaud;
    uint pinoTx;
    uint pinoRx;
};

#endif
