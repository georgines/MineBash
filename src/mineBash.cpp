#include "mineBash.h"

#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "pico/platform.h"

#include "ff.h"

namespace {
constexpr const char* CAMINHO_RAIZ = "/";
constexpr const char* UNIDADE_PADRAO = "0:";
constexpr const char* QUEBRA_LINHA = "\r\n";
}

MineBash::MineBash()
    : cartaoSd(nullptr),
      portaSerial(nullptr),
      portaSerialRegistrada(false),
            cartaoRegistrado(false) {
    diretorioAtual[0] = '/';
    diretorioAtual[1] = 0;
}

void MineBash::registrarPortaSerial(PortaSerial& porta_serial) {
    portaSerial = &porta_serial;
    portaSerialRegistrada = true;
}

void MineBash::registrarCartao(CartaoSD& cartao_sd) {
    cartaoSd = &cartao_sd;
    cartaoRegistrado = true;
}

void MineBash::iniciar() {
    if (!portaSerialRegistrada) {
        return;
    }
    if (!cartaoRegistrado) {
        imprimirMensagem("CartaoSD nao registrado.\n");
        return;
    }
    if (!cartaoSd->montarSistemaArquivos()) {
        imprimirMensagem("Falha ao montar o sistema de arquivos.\n");
        return;
    }
    atualizarDiretorioAtual();
    imprimirMensagem("\nMineBash pronto. Digite 'ajuda' para listar os comandos.\n");
}

void MineBash::processar() {
    if (!portaSerialRegistrada) {
        sleep_ms(1000);
        return;
    }
    if (!cartaoRegistrado) {
        imprimirMensagem("CartaoSD nao registrado.\n");
        sleep_ms(1000);
        return;
    }

    atualizarDiretorioAtual();
    exibirPrompt();

    char linha[TAMANHO_BUFFER_COMANDO];
    bool possui_entrada = lerLinha(linha, sizeof(linha));
    imprimirMensagem("\n");
    if (!possui_entrada) {
        return;
    }

    imprimirDepuracao("Linha bruta recebida: %s\n", linha);
    removerEspacosLaterais(linha);
    if (linha[0] == 0) {
        imprimirDepuracao("Linha vazia apos normalizacao.\n");
        return;
    }

    imprimirDepuracao("Comando normalizado: %s\n", linha);
    executarLinha(linha);
}

void MineBash::exibirPrompt() {
    imprimirMensagem("[%s]$ ", diretorioAtual);
}

bool MineBash::lerLinha(char* destino, size_t capacidade) {
    if (destino == nullptr || capacidade == 0u) {
        return false;
    }

    size_t indice = 0u;
    for (;;) {
        int caractere = getchar_timeout_us(TEMPO_LEITURA_TIMEOUT_US);
        if (caractere == PICO_ERROR_TIMEOUT) {
            tight_loop_contents();
            continue;
        }
        if (caractere == '\r') {
            continue;
        }
        if (caractere == '\n') {
            break;
        }
        if (indice + 1u >= capacidade) {
            continue;
        }
        destino[indice] = static_cast<char>(caractere);
        indice = indice + 1u;
    }

    destino[indice] = 0;
    return indice > 0u;
}

void MineBash::executarLinha(const char* linha) {
    char comando_linha[TAMANHO_BUFFER_COMANDO];
    strncpy(comando_linha, linha, sizeof(comando_linha) - 1u);
    comando_linha[sizeof(comando_linha) - 1u] = 0;

    char token_um[TAMANHO_TOKEN];
    char token_dois[TAMANHO_TOKEN];
    char token_tres[TAMANHO_TOKEN];
    size_t inicio_um = 0u;
    size_t fim_um = 0u;
    size_t inicio_dois = 0u;
    size_t fim_dois = 0u;
    size_t inicio_tres = 0u;
    size_t fim_tres = 0u;

    size_t indice = 0u;
    indice = extrairToken(comando_linha, indice, token_um, sizeof(token_um), inicio_um, fim_um);
    indice = extrairToken(comando_linha, indice, token_dois, sizeof(token_dois), inicio_dois, fim_dois);
    extrairToken(comando_linha, indice, token_tres, sizeof(token_tres), inicio_tres, fim_tres);

    if (token_um[0] == 0) {
        return;
    }

    const char* argumento_pos_primeiro = obterArgumento(linha, fim_um);
    const char* argumento_pos_segundo = obterArgumento(linha, fim_dois);
    const char* argumento_pos_terceiro = obterArgumento(linha, fim_tres);

    if ((strcmp(token_um, "exibir") == 0 && strcmp(token_dois, "ajuda") == 0) || strcmp(token_um, "ajuda") == 0) {
        executarAjuda();
        return;
    }

    if (strcmp(token_um, "listar") == 0) {
    executarListar(argumento_pos_primeiro);
        return;
    }

    if (strcmp(token_um, "remover") == 0) {
    executarRemover(argumento_pos_primeiro);
        return;
    }

    if (strcmp(token_um, "formatar") == 0) {
        executarFormatar();
        return;
    }

    if (strcmp(token_um, "apagar_pasta") == 0 ||
        (strcmp(token_um, "apagar") == 0 && strcmp(token_dois, "pasta") == 0) ||
        (strcmp(token_um, "apagar") == 0 && strcmp(token_dois, "a") == 0 && strcmp(token_tres, "pasta") == 0)) {
        const char* argumento = argumento_pos_primeiro;
        if (strcmp(token_um, "apagar") == 0 && strcmp(token_dois, "pasta") == 0) {
            argumento = argumento_pos_segundo;
        } else if (strcmp(token_um, "apagar") == 0 && strcmp(token_dois, "a") == 0 && strcmp(token_tres, "pasta") == 0) {
            argumento = argumento_pos_terceiro;
        }
        executarApagarPasta(argumento);
        return;
    }

    if (strcmp(token_um, "apagar_arquivo") == 0 ||
        (strcmp(token_um, "apagar") == 0 && (strcmp(token_dois, "arquivo") == 0 || strcmp(token_dois, "arquivos") == 0))) {
        const char* argumento = argumento_pos_primeiro;
        if (strcmp(token_um, "apagar") == 0) {
            argumento = argumento_pos_segundo;
        }
        executarApagarArquivo(argumento);
        return;
    }

    if (strcmp(token_um, "criar_pasta") == 0 || (strcmp(token_um, "criar") == 0 && strcmp(token_dois, "pasta") == 0)) {
        const char* argumento = (strcmp(token_um, "criar") == 0) ? argumento_pos_segundo : argumento_pos_primeiro;
        executarCriarPasta(argumento);
        return;
    }

    if (strcmp(token_um, "criar_arquivo") == 0 || (strcmp(token_um, "criar") == 0 && strcmp(token_dois, "arquivo") == 0)) {
        const char* argumento = (strcmp(token_um, "criar") == 0) ? argumento_pos_segundo : argumento_pos_primeiro;
        executarCriarArquivo(argumento);
        return;
    }

    if (strcmp(token_um, "entrar") == 0) {
        executarEntrar(argumento_pos_primeiro);
        return;
    }

    if (strcmp(token_um, "sair") == 0 || strcmp(token_um, "voltar") == 0) {
        executarSair();
        return;
    }

    if (strcmp(token_um, "escrever_arquivo") == 0 || strcmp(token_um, "escrever") == 0) {
        executarEscreverArquivo(argumento_pos_primeiro);
        return;
    }

    if (strcmp(token_um, "exibir_arquivo") == 0 || strcmp(token_um, "exibir") == 0 || strcmp(token_um, "ler") == 0) {
        const bool comando_antigo_com_token = (strcmp(token_dois, "arquivo") == 0 || strcmp(token_dois, "conteudo") == 0 || strcmp(token_dois, "arquivos") == 0);
        const char* argumento = argumento_pos_primeiro;
        if (strcmp(token_um, "exibir") == 0 || strcmp(token_um, "ler") == 0) {
            argumento = comando_antigo_com_token ? argumento_pos_segundo : argumento_pos_primeiro;
        }
        executarExibirArquivo(argumento);
        return;
    }

    imprimirMensagem("Comando desconhecido. Digite 'ajuda' para ajuda.\n");
}

void MineBash::executarAjuda() {
    imprimirMensagem("\nComandos disponiveis:\n");
    imprimirMensagem("  ajuda                                   - mostra esta lista\n");
    imprimirMensagem("  listar [caminho]                        - lista o conteudo do diretorio\n");
    imprimirMensagem("  remover <caminho>                       - remove arquivo ou diretorio\n");
    imprimirMensagem("  formatar                                - formata a unidade 0:\n");
    imprimirMensagem("  apagar_pasta [-r] <caminho>             - remove uma pasta (use -r para recursivo)\n");
    imprimirMensagem("  apagar_arquivo <caminho>                - remove um arquivo\n");
    imprimirMensagem("  criar_pasta <caminho>                   - cria uma nova pasta\n");
    imprimirMensagem("  criar_arquivo <caminho>                 - cria um arquivo vazio\n");
    imprimirMensagem("  entrar <caminho>                        - entra em um subdiretorio\n");
    imprimirMensagem("  sair                                    - retorna ao diretorio anterior\n");
    imprimirMensagem("  escrever_arquivo [-n] <caminho> \"txt\" - acrescenta texto (use -n para nova linha)\n");
    imprimirMensagem("  exibir_arquivo <caminho>                - mostra o conteudo do arquivo\n\n");
}

void MineBash::executarListar(const char* argumento) {
    if (argumento == nullptr) {
        return;
    }

    char caminho[TAMANHO_AUXILIAR];
    if (argumento[0] == 0) {
        strncpy(caminho, ".", sizeof(caminho) - 1u);
        caminho[1] = 0;
    } else {
        strncpy(caminho, argumento, sizeof(caminho) - 1u);
        caminho[sizeof(caminho) - 1u] = 0;
    }

    ArquivoSd diretorio = cartaoSd->abrir(caminho, MODO_DIRETORIO | MODO_LEITURA);
    if (!diretorio.estaAberto()) {
        imprimirMensagem("Falha ao abrir diretorio.\n");
        return;
    }

    for (;;) {
        ArquivoSd entrada = diretorio.abrirProximaEntrada();
        if (!entrada.estaAberto()) {
            break;
        }

        InformacoesEntradaFat info;
        if (!entrada.obterInformacoes(info)) {
            entrada.fechar();
            continue;
        }

        if (strcmp(info.nome_curto, ".") == 0 || strcmp(info.nome_curto, "..") == 0) {
            entrada.fechar();
            continue;
        }

#if FF_USE_LFN
        const char* nome_exibicao = info.nome_completo[0] != 0 ? info.nome_completo : info.nome_curto;
#else
        const char* nome_exibicao = info.nome_curto;
#endif
        bool eh_diretorio = (info.atributos & AM_DIR) != 0;
    imprimirMensagem("%s%s\n", nome_exibicao, eh_diretorio ? "/" : "");
        entrada.fechar();
    }

    diretorio.fechar();
}

void MineBash::executarRemover(const char* argumento) {
    if (argumento == nullptr || argumento[0] == 0) {
    imprimirMensagem("Informe o caminho a remover.\n");
        return;
    }

    if (cartaoSd->removerArquivo(argumento)) {
    imprimirMensagem("Arquivo removido.\n");
        return;
    }

    if (cartaoSd->removerDiretorio(argumento)) {
    imprimirMensagem("Diretorio removido.\n");
        return;
    }

    imprimirMensagem("Falha ao remover caminho.\n");
}

void MineBash::executarFormatar() {
    static BYTE area_trabalho[TAMANHO_AREA_FORMATACAO];
    memset(area_trabalho, 0, sizeof(area_trabalho));

    ParametrosFormatacaoFat parametros{};
    parametros.formato = FM_ANY;
    parametros.quantidade_fats = 0u;
    parametros.alinhamento_setores = 0u;
    parametros.entradas_raiz = 0u;
    parametros.tamanho_cluster_bytes = 0u;

    imprimirMensagem("Formatando unidade...\n");
    if (!cartaoSd->formatar(UNIDADE_PADRAO, parametros, area_trabalho, sizeof(area_trabalho))) {
    imprimirMensagem("Falha ao formatar: %d\n", cartaoSd->resultadoOperacao());
        return;
    }

    if (!cartaoSd->montarSistemaArquivos()) {
    imprimirMensagem("Falha ao montar apos formatar.\n");
        return;
    }

    atualizarDiretorioAtual();
    imprimirMensagem("Formato concluido.\n");
}

void MineBash::executarApagarPasta(const char* argumento) {
    if (argumento == nullptr) {
        imprimirMensagem("Informe a pasta a remover.\n");
        return;
    }

    const char* cursor = argumento;
    while (*cursor != 0 && std::isspace(static_cast<unsigned char>(*cursor))) {
        cursor++;
    }

    bool recursivo = false;
    const char* caminho_inicio = cursor;

    if (*cursor == '-') {
        const char* inicio_flag = cursor;
        cursor++;
        if (*cursor == 'r' && (cursor[1] == 0 || std::isspace(static_cast<unsigned char>(cursor[1])))) {
            recursivo = true;
            cursor++;
        } else if (*cursor == '-' && strncmp(cursor + 1, "recursiva", 9) == 0) {
            char proximo = cursor[10];
            if (proximo == 0 || std::isspace(static_cast<unsigned char>(proximo))) {
                recursivo = true;
                cursor += 10;
            } else {
                cursor = inicio_flag;
            }
        } else {
            cursor = inicio_flag;
        }

        if (recursivo) {
            while (*cursor != 0 && std::isspace(static_cast<unsigned char>(*cursor))) {
                cursor++;
            }
            caminho_inicio = cursor;
        }
    }

    if (!recursivo) {
        caminho_inicio = cursor;
    }

    char caminho[TAMANHO_AUXILIAR];
    strncpy(caminho, caminho_inicio, sizeof(caminho) - 1u);
    caminho[sizeof(caminho) - 1u] = 0;

    // Remove espaços em branco ao final
    size_t tamanho = strlen(caminho);
    while (tamanho > 0u && std::isspace(static_cast<unsigned char>(caminho[tamanho - 1u]))) {
        caminho[tamanho - 1u] = 0;
        tamanho--;
    }

    if (caminho[0] == 0) {
        imprimirMensagem("Informe a pasta a remover.\n");
        return;
    }

    bool sucesso = recursivo ? cartaoSd->removerDiretorioRecursivo(caminho)
                             : cartaoSd->removerDiretorio(caminho);

    if (sucesso) {
        imprimirMensagem("Pasta removida.\n");
    } else {
        imprimirMensagem("Falha ao remover a pasta.\n");
    }
}

void MineBash::executarCriarPasta(const char* argumento) {
    if (argumento == nullptr || argumento[0] == 0) {
    imprimirMensagem("Informe a pasta a criar.\n");
        return;
    }

    size_t indice_inicio = pularEspacos(argumento, 0u);
    const char* argumento_normalizado = argumento + indice_inicio;

    char caminho_informado[TAMANHO_AUXILIAR];
    strncpy(caminho_informado, argumento_normalizado, sizeof(caminho_informado) - 1u);
    caminho_informado[sizeof(caminho_informado) - 1u] = 0;

    size_t tamanho_caminho = strlen(caminho_informado);
    while (tamanho_caminho > 0u) {
        char caractere_final = caminho_informado[tamanho_caminho - 1u];
        if (caractere_final != ' ' && caractere_final != '\t') {
            break;
        }
        caminho_informado[tamanho_caminho - 1u] = 0;
        tamanho_caminho = tamanho_caminho - 1u;
    }

    if (caminho_informado[0] == 0) {
        imprimirMensagem("Informe a pasta a criar.\n");
        return;
    }

    bool caminho_absoluto = false;
    if (caminho_informado[0] == '/' || caminho_informado[0] == '\\') {
        caminho_absoluto = true;
    } else if (strlen(caminho_informado) >= 2u && caminho_informado[1] == ':') {
        caminho_absoluto = true;
    }

    char caminho_destino[TAMANHO_AUXILIAR];
    if (caminho_absoluto) {
        strncpy(caminho_destino, caminho_informado, sizeof(caminho_destino) - 1u);
        caminho_destino[sizeof(caminho_destino) - 1u] = 0;
    } else {
        bool diretorio_raiz = (diretorioAtual[0] == '/' && diretorioAtual[1] == 0);
        int resultado_formatacao = diretorio_raiz
            ? snprintf(caminho_destino, sizeof(caminho_destino), "/%s", caminho_informado)
            : snprintf(caminho_destino, sizeof(caminho_destino), "%s/%s", diretorioAtual, caminho_informado);
        if (resultado_formatacao < 0 || resultado_formatacao >= static_cast<int>(sizeof(caminho_destino))) {
            imprimirMensagem("Caminho muito longo.\n");
            return;
        }
    }

    if (cartaoSd->criarDiretorio(caminho_destino)) {
    imprimirMensagem("Pasta criada com sucesso.\n");
    } else {
    imprimirMensagem("Falha ao criar a pasta.\n");
    }
}

void MineBash::executarApagarArquivo(const char* argumento) {
    if (argumento == nullptr || argumento[0] == 0) {
        imprimirMensagem("Informe o arquivo a remover.\n");
        return;
    }

    if (cartaoSd->removerArquivo(argumento)) {
        imprimirMensagem("Arquivo removido.\n");
    } else {
        imprimirMensagem("Falha ao remover o arquivo. Codigo erro: %d\n", cartaoSd->resultadoOperacao());
    }
}

void MineBash::executarCriarArquivo(const char* argumento) {
    if (argumento == nullptr || argumento[0] == 0) {
    imprimirMensagem("Informe o arquivo a criar.\n");
        return;
    }

    ArquivoSd arquivo = cartaoSd->abrir(argumento, MODO_ESCRITA);
    if (!arquivo.estaAberto()) {
    imprimirMensagem("Falha ao criar o arquivo.\n");
        return;
    }
    arquivo.fechar();
    imprimirMensagem("Arquivo criado.\n");
}

void MineBash::executarEntrar(const char* argumento) {
    if (argumento == nullptr || argumento[0] == 0) {
    imprimirMensagem("Informe o diretorio destino.\n");
        return;
    }

    if (!cartaoSd->alterarDiretorioAtual(argumento)) {
    imprimirMensagem("Falha ao entrar no diretorio.\n");
        return;
    }

    atualizarDiretorioAtual();
}

void MineBash::executarSair() {
    if (!cartaoSd->alterarDiretorioAtual("..")) {
        imprimirMensagem("Falha ao retornar diretorio.\n");
        return;
    }
    atualizarDiretorioAtual();
}

void MineBash::executarEscreverArquivo(const char* argumento) {
    if (argumento == nullptr || argumento[0] == 0) {
        imprimirMensagem("Informe caminho e texto.\n");
        return;
    }

    size_t indice_origem = pularEspacos(argumento, 0u);
    bool inserir_quebra_linha = false;

    if (argumento[indice_origem] == '-') {
        size_t indice_flag = indice_origem + 1u;
        char identificador_flag = argumento[indice_flag];
        if (identificador_flag == 'n' || identificador_flag == 'N') {
            inserir_quebra_linha = true;
            indice_flag = indice_flag + 1u;
            if (argumento[indice_flag] == 0) {
                imprimirMensagem("Informe caminho e texto.\n");
                return;
            }
            if (argumento[indice_flag] == ' ' || argumento[indice_flag] == '\t') {
                indice_origem = pularEspacos(argumento, indice_flag);
            } else {
                indice_origem = indice_flag;
            }
        }
    }

    char caminho[TAMANHO_AUXILIAR];
    char conteudo[TAMANHO_BUFFER_COMANDO];
    size_t indice_caminho = 0u;

    while (argumento[indice_origem] != 0 && argumento[indice_origem] != ' ' && argumento[indice_origem] != '\t') {
        if (indice_caminho + 1u < sizeof(caminho)) {
            caminho[indice_caminho] = argumento[indice_origem];
            indice_caminho = indice_caminho + 1u;
        }
        indice_origem = indice_origem + 1u;
    }
    caminho[indice_caminho] = 0;

    indice_origem = pularEspacos(argumento, indice_origem);
    if (argumento[indice_origem] != '"') {
        imprimirMensagem("Informe texto entre aspas.\n");
        return;
    }

    indice_origem = indice_origem + 1u;
    size_t indice_conteudo = 0u;
    bool encontrou_fechamento = false;
    bool conteudo_cabia = true;
    while (argumento[indice_origem] != 0) {
        char caractere_atual = argumento[indice_origem];
        if (caractere_atual == '"') {
            encontrou_fechamento = true;
            indice_origem = indice_origem + 1u;
            break;
        }
        if (indice_conteudo + 1u < sizeof(conteudo)) {
            conteudo[indice_conteudo] = caractere_atual;
            indice_conteudo = indice_conteudo + 1u;
        } else {
            conteudo_cabia = false;
        }
        indice_origem = indice_origem + 1u;
    }
    conteudo[indice_conteudo] = 0;

    if (!encontrou_fechamento) {
        imprimirMensagem("Texto sem fechamento de aspas.\n");
        return;
    }

    if (!conteudo_cabia) {
        imprimirMensagem("Texto muito longo.\n");
        return;
    }

    indice_origem = pularEspacos(argumento, indice_origem);
    if (argumento[indice_origem] != 0) {
        imprimirMensagem("Argumentos adicionais nao suportados apos o texto.\n");
        return;
    }

    if (caminho[0] == 0 || conteudo[0] == 0) {
        imprimirMensagem("Informe caminho e texto.\n");
        return;
    }

    ArquivoSd arquivo = cartaoSd->abrir(caminho, MODO_LEITURA | MODO_ESCRITA | MODO_ACRESCENTAR);
    if (!arquivo.estaAberto()) {
        imprimirMensagem("Falha ao abrir arquivo para escrita.\n");
        return;
    }

    bool escreveu = true;

    if (inserir_quebra_linha) {
        long tamanho_atual = arquivo.tamanho();
        if (tamanho_atual < 0) {
            escreveu = false;
        } else if (tamanho_atual > 0) {
            bool precisa_inserir_quebra = true;

            bool posicionou_no_final = arquivo.buscar(tamanho_atual - 1);
            if (posicionou_no_final) {
                uint8_t ultimo_byte = 0u;
                size_t bytes_lidos = arquivo.lerBytes(&ultimo_byte, 1u);
                if (bytes_lidos == 1u) {
                    if (ultimo_byte == '\n' || ultimo_byte == '\r') {
                        precisa_inserir_quebra = false;
                    }
                }
            }

            if (precisa_inserir_quebra) {
                escreveu = arquivo.escreverLinha(QUEBRA_LINHA);
            }
        }
    }

    if (escreveu) {
        escreveu = arquivo.escreverLinha(conteudo);
    }

    arquivo.fechar();

    if (escreveu) {
        imprimirMensagem("Texto gravado.\n");
    } else {
        imprimirMensagem("Falha ao gravar texto.\n");
    }
}

void MineBash::executarExibirArquivo(const char* argumento) {
    if (argumento == nullptr || argumento[0] == 0) {
    imprimirMensagem("Informe o arquivo a exibir.\n");
        return;
    }

    ArquivoSd arquivo = cartaoSd->abrir(argumento, MODO_LEITURA);
    if (!arquivo.estaAberto()) {
    imprimirMensagem("Falha ao abrir o arquivo.\n");
        return;
    }

    uint8_t buffer[TAMANHO_AUXILIAR];
    for (;;) {
        size_t lidos = arquivo.lerBytes(buffer, sizeof(buffer) - 1u);
        if (lidos == 0u) {
            break;
        }
        buffer[lidos] = 0;
    imprimirMensagem("%s", reinterpret_cast<char*>(buffer));
    }
    imprimirMensagem("\n");
    arquivo.fechar();
}

void MineBash::atualizarDiretorioAtual() {
    if (!cartaoRegistrado) {
        strncpy(diretorioAtual, CAMINHO_RAIZ, sizeof(diretorioAtual) - 1u);
        diretorioAtual[sizeof(diretorioAtual) - 1u] = 0;
        return;
    }

    char caminho_completo[TAMANHO_DIRETORIO];
    if (!cartaoSd->obterDiretorioAtual(caminho_completo, sizeof(caminho_completo))) {
        strncpy(diretorioAtual, CAMINHO_RAIZ, sizeof(diretorioAtual) - 1u);
        diretorioAtual[sizeof(diretorioAtual) - 1u] = 0;
        return;
    }

    const char* caminho_tratado = caminho_completo;
    if (caminho_tratado[0] != 0 && caminho_tratado[1] == ':') {
        caminho_tratado = caminho_tratado + 2;
    }

    if (caminho_tratado[0] == 0) {
        strncpy(diretorioAtual, CAMINHO_RAIZ, sizeof(diretorioAtual) - 1u);
        diretorioAtual[sizeof(diretorioAtual) - 1u] = 0;
        return;
    }

    if (caminho_tratado[0] != '/') {
        diretorioAtual[0] = '/';
        strncpy(diretorioAtual + 1u, caminho_tratado, sizeof(diretorioAtual) - 2u);
        diretorioAtual[sizeof(diretorioAtual) - 1u] = 0;
        return;
    }

    strncpy(diretorioAtual, caminho_tratado, sizeof(diretorioAtual) - 1u);
    diretorioAtual[sizeof(diretorioAtual) - 1u] = 0;
}

void MineBash::imprimirMensagem(const char* formato, ...) {
    if (formato == nullptr || !portaSerialRegistrada) {
        return;
    }

    char buffer[TAMANHO_BUFFER_COMANDO];
    va_list lista_argumentos;
    va_start(lista_argumentos, formato);
    vsnprintf(buffer, sizeof(buffer), formato, lista_argumentos);
    va_end(lista_argumentos);

    portaSerial->enviarTexto(buffer);
}

void MineBash::imprimirDepuracao(const char* formato, ...) {
#if MINEBASH_DEPURACAO_ATIVA
    if (formato == nullptr || !portaSerialRegistrada) {
        return;
    }

    portaSerial->enviarTexto("[DBG] ");

    char buffer[TAMANHO_BUFFER_COMANDO];
    va_list argumentos;
    va_start(argumentos, formato);
    vsnprintf(buffer, sizeof(buffer), formato, argumentos);
    va_end(argumentos);

    portaSerial->enviarTexto(buffer);
#else
    (void)formato;
#endif
}

const char* MineBash::obterArgumento(const char* linha, size_t indice_inicio) {
    size_t indice = pularEspacos(linha, indice_inicio);
    return linha + indice;
}

size_t MineBash::extrairToken(const char* linha, size_t indice_inicio, char* destino, size_t capacidade, size_t& indice_token_inicio, size_t& indice_token_fim) {
    indice_token_inicio = pularEspacos(linha, indice_inicio);
    if (linha[indice_token_inicio] == 0) {
        if (capacidade > 0u) {
            destino[0] = 0;
        }
        indice_token_fim = indice_token_inicio;
        return indice_token_inicio;
    }

    size_t indice = indice_token_inicio;
    size_t indice_destino = 0u;
    while (linha[indice] != 0 && linha[indice] != ' ' && linha[indice] != '\t') {
        if (indice_destino + 1u < capacidade) {
            destino[indice_destino] = static_cast<char>(tolower(static_cast<unsigned char>(linha[indice])));
            indice_destino = indice_destino + 1u;
        }
        indice = indice + 1u;
    }

    if (capacidade > 0u) {
        if (indice_destino < capacidade) {
            destino[indice_destino] = 0;
        } else {
            destino[capacidade - 1u] = 0;
        }
    }

    indice_token_fim = indice;
    if (linha[indice] == 0) {
        return indice;
    }
    return indice + 1u;
}

size_t MineBash::pularEspacos(const char* linha, size_t indice_inicio) {
    size_t indice = indice_inicio;
    while (linha[indice] == ' ' || linha[indice] == '\t') {
        indice = indice + 1u;
    }
    return indice;
}

void MineBash::removerEspacosLaterais(char* texto) {
    if (texto == nullptr) {
        return;
    }

    size_t tamanho = strlen(texto);
    size_t inicio = 0u;
    while (texto[inicio] == ' ' || texto[inicio] == '\t') {
        inicio = inicio + 1u;
    }

    size_t fim = tamanho;
    while (fim > inicio && (texto[fim - 1u] == ' ' || texto[fim - 1u] == '\t')) {
        fim = fim - 1u;
    }

    size_t indice_destino = 0u;
    for (size_t indice = inicio; indice < fim; indice = indice + 1u) {
        texto[indice_destino] = texto[indice];
        indice_destino = indice_destino + 1u;
    }
    texto[indice_destino] = 0;
}

