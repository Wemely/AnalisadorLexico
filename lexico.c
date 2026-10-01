#include "automato.h"

static void copiarTrecho(char *destino, size_t tamanho, const char *origem, size_t comprimento) {
    if (tamanho == 0) {
        return;
    }
    if (comprimento >= tamanho) {
        comprimento = tamanho - 1;
    }
    memcpy(destino, origem, comprimento);
    destino[comprimento] = '\0';
}

void inicializarAnalise(AnaliseLexica *analise) {
    if (analise != NULL) {
        memset(analise, 0, sizeof(*analise));
    }
}

static void adicionarResultado(AnaliseLexica *analise, int linha,
                               const char *lexema, const char *rotulo,
                               int tokenId, bool erro) {
    if (analise == NULL || analise->qtdRegistros >= MAX_REGISTROS) {
        return;
    }

    RegistroSimbolo *registro = &analise->registros[analise->qtdRegistros++];
    registro->linha = linha;
    snprintf(registro->identificador, sizeof(registro->identificador), "%s", lexema);
    snprintf(registro->rotulo, sizeof(registro->rotulo), "%s", rotulo);
    registro->tokenId = tokenId;
    registro->erro = erro;

    if (analise->qtdFita < MAX_REGISTROS) {
        snprintf(analise->fita[analise->qtdFita], sizeof(analise->fita[0]), "%s", rotulo);
        analise->qtdFita++;
    }
    if (erro) {
        analise->qtdErros++;
    }
}

static bool lerArquivoInteiro(const char *arquivo, char **conteudo, size_t *tamanho) {
    FILE *f = fopen(arquivo, "rb");
    if (f == NULL) {
        fprintf(stderr, "Erro: não foi possível abrir a fonte '%s'.\n", arquivo);
        return false;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return false;
    }
    long fim = ftell(f);
    if (fim < 0) {
        fclose(f);
        return false;
    }
    rewind(f);

    size_t bytes = (size_t)fim;
    char *buffer = malloc(bytes + 1);
    if (buffer == NULL) {
        fclose(f);
        fprintf(stderr, "Erro: memória insuficiente para ler '%s'.\n", arquivo);
        return false;
    }
    size_t lidos = fread(buffer, 1, bytes, f);
    bool houveErro = ferror(f) != 0;
    fclose(f);
    if (lidos != bytes && houveErro) {
        free(buffer);
        return false;
    }
    buffer[lidos] = '\0';
    *conteudo = buffer;
    *tamanho = lidos;
    return true;
}

bool analisarFonte(const char *arquivo, const Automato *afd, AnaliseLexica *analise) {
    if (arquivo == NULL || afd == NULL || analise == NULL || afd->estadoInicial < 0) {
        return false;
    }

    inicializarAnalise(analise);
    char *conteudo = NULL;
    size_t tamanho = 0;
    if (!lerArquivoInteiro(arquivo, &conteudo, &tamanho)) {
        return false;
    }

    size_t posicao = 0;
    int linha = 1;
    while (posicao < tamanho) {
        unsigned char caractere = (unsigned char)conteudo[posicao];
        if (isspace(caractere)) {
            if (caractere == '\n') {
                linha++;
            }
            posicao++;
            continue;
        }

        size_t inicio = posicao;
        int estado = afd->estadoInicial;
        size_t tamanhoAceito = 0;
        int tokenAceito = TOKEN_NENHUM;

        while (posicao < tamanho &&
               !isspace((unsigned char)conteudo[posicao])) {
            int destino = buscarTransicao(afd, estado, conteudo[posicao]);
            if (destino < 0) {
                break;
            }
            estado = destino;
            posicao++;

            if (estado != afd->estadoErro && afd->estados[estado].final) {
                tamanhoAceito = posicao - inicio;
                tokenAceito = afd->estados[estado].tokenId;
            }
        }

        if (tamanhoAceito > 0) {
            char lexema[MAX_LEXEMA];
            copiarTrecho(lexema, sizeof(lexema), conteudo + inicio, tamanhoAceito);
            const char *rotulo = obterRotuloToken(afd, tokenAceito);
            adicionarResultado(analise, linha, lexema, rotulo, tokenAceito, false);
            /* O trecho lido depois do último estado final será reprocessado. */
            posicao = inicio + tamanhoAceito;
        } else {
            char lexema[2] = {conteudo[inicio], '\0'};
            adicionarResultado(analise, linha, lexema, "X", TOKEN_NENHUM, true);
            posicao = inicio + 1;
        }
    }

    free(conteudo);
    return true;
}

void imprimirAnalise(const AnaliseLexica *analise) {
    if (analise == NULL) {
        return;
    }

    printf("\n-----------------------  FITA -----------------------\nFITA: ");
    for (size_t i = 0; i < analise->qtdFita; i++) {
        if (i > 0) {
            printf(" ");
        }
        printf("%s", analise->fita[i]);
    }
    printf(" $\n");

    printf("\n-----------------------  TABELA DE SÍMBOLOS -----------------------\n");
    printf("Linha |    Identificador       | Rótulo\n");
    printf("------+------------------------+----------------\n");
    for (size_t i = 0; i < analise->qtdRegistros; i++) {
        const RegistroSimbolo *registro = &analise->registros[i];
        printf("%5d | %-22s | %s%s\n", registro->linha,
               registro->identificador, registro->rotulo,
               registro->erro ? " (erro)" : "");
    }
    printf("\nErros: %zu \n", analise->qtdErros);
}

bool salvarAnalise(const AnaliseLexica *analise, const char *arquivoFita,
                   const char *arquivoTabela) {
    if (analise == NULL || arquivoFita == NULL || arquivoTabela == NULL) {
        return false;
    }

    FILE *fita = fopen(arquivoFita, "w");
    if (fita == NULL) {
        fprintf(stderr, "Erro: não foi possível criar '%s'.\n", arquivoFita);
        return false;
    }
    fprintf(fita, "FITA: ");
    for (size_t i = 0; i < analise->qtdFita; i++) {
        if (i > 0) {
            fputc(' ', fita);
        }
        fputs(analise->fita[i], fita);
    }
    fprintf(fita, " $\n");
    fclose(fita);

    FILE *tabela = fopen(arquivoTabela, "w");
    if (tabela == NULL) {
        fprintf(stderr, "Erro: não foi possível criar '%s'.\n", arquivoTabela);
        return false;
    }
    fprintf(tabela, "Linha | Identificador (lexema) | Rótulo\n");
    fprintf(tabela, "------+------------------------+----------------\n");
    for (size_t i = 0; i < analise->qtdRegistros; i++) {
        const RegistroSimbolo *registro = &analise->registros[i];
        fprintf(tabela, "%5d | %-22s | %s%s\n", registro->linha,
                registro->identificador, registro->rotulo,
                registro->erro ? " (erro)" : "");
    }
    fclose(tabela);
    return true;
}
