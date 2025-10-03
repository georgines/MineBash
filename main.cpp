#include <stdint.h>

#include "CartaoSD.h"
#include "mineBash.h"
#include "PortaSerial.h"

static spi_inst_t *const SPI_CARTAO = spi0;
static constexpr uint8_t PINO_SPI_MISO_CARTAO = 16u;
static constexpr uint8_t PINO_SPI_MOSI_CARTAO = 19u;
static constexpr uint8_t PINO_SPI_SCK_CARTAO = 18u;
static constexpr uint8_t PINO_SPI_CS_CARTAO = 17u;

static uart_inst_t * const INTERFACE_UART = uart1;
static constexpr uint32_t TAXA_BPS_UART = 115200u;
static constexpr uint8_t PINO_UART_TX = 8u;
static constexpr uint8_t PINO_UART_RX = 9u;

int main()
{
    CartaoSD cartao(SPI_CARTAO, PINO_SPI_MISO_CARTAO, PINO_SPI_MOSI_CARTAO, PINO_SPI_SCK_CARTAO, PINO_SPI_CS_CARTAO);
    PortaSerial porta_serial(INTERFACE_UART, TAXA_BPS_UART, PINO_UART_TX, PINO_UART_RX);
    porta_serial.iniciar();

    MineBash console;
    console.registrarPortaSerial(porta_serial);
    console.registrarCartao(cartao);
    console.iniciar();

    while (true)
    {
        console.processar();
    }

    return 0;
}
