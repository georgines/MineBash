#ifndef MINEBASH_H
#define MINEBASH_H

#include <cstddef>
#include <cstdint>

#include "pico/stdlib.h"

#include "CartaoSD.h"
#include "PortaSerial.h"

#ifndef MINEBASH_DEPURACAO_ATIVA
#define MINEBASH_DEPURACAO_ATIVA 0
#endif
class MineBash {
public:
    MineBash();

    void registrarPortaSerial(PortaSerial& porta_serial);
    void registrarCartao(CartaoSD &cartao_sd);
    void iniciar();
    void processar();

private:
    static constexpr size_t TAMANHO_BUFFER_COMANDO = 256u;
    static constexpr size_t TAMANHO_TOKEN = 32u;
    static constexpr size_t TAMANHO_DIRETORIO = 256u;
    static constexpr int64_t TEMPO_LEITURA_TIMEOUT_US = 1000;
    static constexpr size_t TAMANHO_AUXILIAR = 512u;
    static constexpr size_t TAMANHO_AREA_FORMATACAO = 4096u;

    CartaoSD *cartaoSd;
    PortaSerial* portaSerial;
    bool portaSerialRegistrada;
    bool cartaoRegistrado;
    char diretorioAtual[TAMANHO_DIRETORIO];

    void exibirPrompt();
    bool lerLinha(char *destino, size_t capacidade);
    void executarLinha(const char *linha);
    void executarAjuda();
    void executarListar(const char *argumento);
    void executarRemover(const char *argumento);
    void executarFormatar();
    void executarApagarPasta(const char *argumento);
    void executarCriarPasta(const char *argumento);
    void executarApagarArquivo(const char *argumento);
    void executarCriarArquivo(const char *argumento);
    void executarEntrar(const char *argumento);
    void executarSair();
    void executarEscreverArquivo(const char *argumento);
    void executarExibirArquivo(const char *argumento);
    void atualizarDiretorioAtual();
    const char *obterArgumento(const char *linha, size_t indice_inicio);
    size_t extrairToken(const char *linha, size_t indice_inicio, char *destino, size_t capacidade, size_t &indice_token_inicio, size_t &indice_token_fim);
    size_t pularEspacos(const char *linha, size_t indice_inicio);
    void removerEspacosLaterais(char *texto);
    void imprimirMensagem(const char *formato, ...);
    void imprimirDepuracao(const char *formato, ...);
};

#endif
