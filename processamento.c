#include "automato.h"

/* Mapa usado para reaproveitar os estados das variáveis da gramática. */
typedef struct {
    char nome[64];
    int estadoId;
} MapaEstado;

static MapaEstado mapa[MAX_ESTADOS];
static int qtdMapa;
static int tokenGramatica;

static void inicializarMapa(void) {
    memset(mapa, 0, sizeof(mapa));
    qtdMapa = 0;
    tokenGramatica = TOKEN_NENHUM;
}

static int obterOuCriarEstado(Automato *afnd, const char *nome) {
    for (int i = 0; i < qtdMapa; i++) {
        if (strcmp(mapa[i].nome, nome) == 0) {
            return mapa[i].estadoId;
        }
    }

    if (qtdMapa >= MAX_ESTADOS) {
        return -1;
    }
    int id = adicionarEstado(afnd, false);
    if (id < 0) {
        return -1;
    }
    snprintf(mapa[qtdMapa].nome, sizeof(mapa[qtdMapa].nome), "%s", nome);
    mapa[qtdMapa].estadoId = id;
    qtdMapa++;
    return id;
}

static void gerarRotuloToken(const char *token, char *rotulo, size_t tamanho) {
    if (tamanho == 0) {
        return;
    }
    size_t pos = 0;
    const char prefixo[] = "T_";
    for (size_t i = 0; prefixo[i] != '\0' && pos + 1 < tamanho; i++) {
        rotulo[pos++] = prefixo[i];
    }

    for (size_t i = 0; token[i] != '\0' && pos + 1 < tamanho; i++) {
        unsigned char c = (unsigned char)token[i];
        if (isalnum(c) || c == '_') {
            rotulo[pos++] = (char)toupper(c);
        } else if (pos + 3 < tamanho) {
            static const char hex[] = "0123456789ABCDEF";
            rotulo[pos++] = '_';
            rotulo[pos++] = hex[c >> 4];
            rotulo[pos++] = hex[c & 0x0F];
        }
    }
    rotulo[pos] = '\0';
}

bool carregarArquivo(const char *arquivo, Automato *afnd) {
    if (arquivo == NULL || afnd == NULL) {
        return false;
    }

    FILE *f = fopen(arquivo, "r");
    if (f == NULL) {
        fprintf(stderr, "Erro: não foi possível abrir a especificação '%s'.\n", arquivo);
        return false;
    }

    inicializarMapa();
    /* A gramática regular do projeto usa S como símbolo inicial. */
    if (obterOuCriarEstado(afnd, "S") < 0) {
        fclose(f);
        return false;
    }

    char linha[MAX_LINHA];
    int numeroLinha = 0;
    while (fgets(linha, sizeof(linha), f) != NULL) {
        numeroLinha++;
        tiraEspaco(linha);
        if (linha[0] == '\0' || linha[0] == '#') {
            continue;
        }

        if (linha[0] == '<') {
            processarGramatica(linha, afnd);
        } else {
            processarToken(linha, afnd);
        }
    }
    fclose(f);

    if (afnd->qtdEstados == 0 || afnd->estadoInicial < 0) {
        fprintf(stderr, "Erro: a especificação '%s' não criou estados.\n", arquivo);
        return false;
    }
    (void)numeroLinha;
    return true;
}

void processarToken(char *token, Automato *afnd) {
    if (token == NULL || afnd == NULL) {
        return;
    }
    tiraEspaco(token);
    if (token[0] == '\0') {
        return;
    }

    char rotulo[MAX_ROTULO];
    gerarRotuloToken(token, rotulo, sizeof(rotulo));
    int tokenId = registrarToken(afnd, token, rotulo);
    if (tokenId < 0) {
        return;
    }

    int atual = afnd->estadoInicial;
    for (size_t i = 0; token[i] != '\0'; i++) {
        int proximo = adicionarEstado(afnd, false);
        if (proximo < 0) {
            return;
        }
        adicionarTransicao(afnd, atual, proximo, token[i]);
        atual = proximo;
    }
    definirEstadoToken(afnd, atual, tokenId);
}

static bool extrairNomeVariavel(const char *texto, char *nome, size_t tamanho) {
    if (texto == NULL || nome == NULL || tamanho == 0) {
        return false;
    }
    const char *inicio = strchr(texto, '<');
    const char *fim = inicio != NULL ? strchr(inicio + 1, '>') : NULL;
    if (inicio == NULL || fim == NULL || fim <= inicio + 1) {
        return false;
    }
    size_t comprimento = (size_t)(fim - inicio - 1);
    if (comprimento >= tamanho) {
        comprimento = tamanho - 1;
    }
    memcpy(nome, inicio + 1, comprimento);
    nome[comprimento] = '\0';
    return true;
}

static bool extrairProducaoTerminal(const char *producao, char *simbolo,
                                    char *variavel, size_t tamanhoVariavel) {
    if (producao == NULL || simbolo == NULL || variavel == NULL || tamanhoVariavel == 0) {
        return false;
    }
    *simbolo = '\0';
    variavel[0] = '\0';

    if (producao[0] == '\0') {
        return false;
    }
    *simbolo = producao[0];

    const char *inicio = strchr(producao + 1, '<');
    if (inicio != NULL) {
        const char *fim = strchr(inicio + 1, '>');
        if (fim == NULL || fim <= inicio + 1) {
            return false;
        }
        size_t comprimento = (size_t)(fim - inicio - 1);
        if (comprimento >= tamanhoVariavel) {
            comprimento = tamanhoVariavel - 1;
        }
        memcpy(variavel, inicio + 1, comprimento);
        variavel[comprimento] = '\0';
        return true;
    }

    /* Também aceita a forma compacta aA usada no enunciado. */
    if (producao[1] != '\0' && producao[2] == '\0' && isupper((unsigned char)producao[1])) {
        variavel[0] = producao[1];
        variavel[1] = '\0';
        return true;
    }

    return false;
}

void processarGramatica(char *linha, Automato *afnd) {
    if (linha == NULL || afnd == NULL) {
        return;
    }

    char *separador = strstr(linha, "::=");
    if (separador == NULL) {
        fprintf(stderr, "Aviso: produção ignorada (faltou ::=): %s\n", linha);
        return;
    }

    char esquerda[64];
    size_t tamanhoEsquerda = (size_t)(separador - linha);
    if (tamanhoEsquerda >= sizeof(esquerda)) {
        tamanhoEsquerda = sizeof(esquerda) - 1;
    }
    memcpy(esquerda, linha, tamanhoEsquerda);
    esquerda[tamanhoEsquerda] = '\0';
    tiraEspaco(esquerda);

    char nomeVariavel[64];
    if (!extrairNomeVariavel(esquerda, nomeVariavel, sizeof(nomeVariavel))) {
        fprintf(stderr, "Aviso: variável inválida na produção: %s\n", linha);
        return;
    }

    int estadoOrigem = obterOuCriarEstado(afnd, nomeVariavel);
    if (estadoOrigem < 0) {
        fprintf(stderr, "Aviso: não foi possível criar o estado <%s>.\n", nomeVariavel);
        return;
    }

    if (tokenGramatica == TOKEN_NENHUM) {
        tokenGramatica = registrarToken(afnd, "<gramatica-regular>", "IDENTIFICADOR");
    }

    char direita[MAX_LINHA];
    snprintf(direita, sizeof(direita), "%s", separador + 3);
    char *producao = strtok(direita, "|");
    while (producao != NULL) {
        tiraEspaco(producao);
        if (strcmp(producao, "ε") == 0 || strcmp(producao, "EPSILON") == 0 ||
            strcmp(producao, "epsilon") == 0) {
            definirEstadoToken(afnd, estadoOrigem, tokenGramatica);
        } else {
            char simbolo;
            char proximaVariavel[64];
            if (extrairProducaoTerminal(producao, &simbolo, proximaVariavel,
                                        sizeof(proximaVariavel))) {
                int destino = obterOuCriarEstado(afnd, proximaVariavel);
                if (destino >= 0) {
                    adicionarTransicao(afnd, estadoOrigem, destino, simbolo);
                }
            } else if (strlen(producao) == 1) {
                int estadoFinal = adicionarEstado(afnd, false);
                if (estadoFinal >= 0) {
                    adicionarTransicao(afnd, estadoOrigem, estadoFinal, producao[0]);
                    definirEstadoToken(afnd, estadoFinal, tokenGramatica);
                }
            } else {
                fprintf(stderr, "Aviso: produção ignorada: %s\n", producao);
            }
        }
        producao = strtok(NULL, "|");
    }
}
