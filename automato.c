#include "automato.h"

static void copiarTexto(char *destino, size_t tamanho, const char *origem) {
    if (tamanho == 0) {
        return;
    }
    if (origem == NULL) {
        destino[0] = '\0';
        return;
    }
    snprintf(destino, tamanho, "%s", origem);
}

void iniciaAutomato(Automato *a) {
    if (a == NULL) {
        return;
    }
    memset(a, 0, sizeof(*a));
    a->estadoInicial = -1;
    a->estadoErro = -1;
}

int adicionarEstado(Automato *a, bool ehFinal) {
    if (a == NULL || a->qtdEstados >= MAX_ESTADOS) {
        return -1;
    }

    int id = a->qtdEstados++;
    Estado *estado = &a->estados[id];
    memset(estado, 0, sizeof(*estado));
    estado->id = id;
    estado->final = ehFinal;
    estado->tokenId = TOKEN_NENHUM;

    if (a->estadoInicial < 0) {
        a->estadoInicial = id;
    }
    return id;
}

void definirEstadoToken(Automato *a, int estado, int tokenId) {
    if (a == NULL || estado < 0 || estado >= a->qtdEstados) {
        return;
    }
    if (tokenId < 0 || tokenId >= a->qtdTokens) {
        return;
    }

    Estado *alvo = &a->estados[estado];
    alvo->final = true;
    /* Menor id representa maior prioridade: tokens aparecem primeiro no arquivo. */
    if (alvo->tokenId == TOKEN_NENHUM || tokenId < alvo->tokenId) {
        alvo->tokenId = tokenId;
    }
}

void adicionarTransicao(Automato *a, int origem, int destino, char simbolo) {
    if (a == NULL || origem < 0 || origem >= a->qtdEstados ||
        destino < 0 || destino >= a->qtdEstados || simbolo == '\0') {
        return;
    }

    Estado *estado = &a->estados[origem];
    if (estado->qtdTransicoes >= MAX_TRANSICOES) {
        return;
    }

    estado->transicoes[estado->qtdTransicoes].simbolo = simbolo;
    estado->transicoes[estado->qtdTransicoes].destino = destino;
    estado->qtdTransicoes++;
    adicionarSimboloAlfabeto(a, simbolo);
}

void adicionarSimboloAlfabeto(Automato *a, char simbolo) {
    if (a == NULL || simbolo == '\0') {
        return;
    }

    for (int i = 0; i < a->qtdSimbolos; i++) {
        if (a->alfabeto[i] == simbolo) {
            return;
        }
    }
    if (a->qtdSimbolos < MAX_SIMBOLOS) {
        a->alfabeto[a->qtdSimbolos++] = simbolo;
    }
}

int buscarTransicao(const Automato *a, int estado, char simbolo) {
    if (a == NULL || estado < 0 || estado >= a->qtdEstados) {
        return -1;
    }
    const Estado *origem = &a->estados[estado];
    for (int i = 0; i < origem->qtdTransicoes; i++) {
        if (origem->transicoes[i].simbolo == simbolo) {
            return origem->transicoes[i].destino;
        }
    }
    return -1;
}

int registrarToken(Automato *a, const char *expressao, const char *rotulo) {
    if (a == NULL || expressao == NULL || rotulo == NULL || expressao[0] == '\0' ||
        rotulo[0] == '\0') {
        return TOKEN_NENHUM;
    }

    for (int i = 0; i < a->qtdTokens; i++) {
        if (strcmp(a->tokens[i].expressao, expressao) == 0 &&
            strcmp(a->tokens[i].rotulo, rotulo) == 0) {
            return a->tokens[i].id;
        }
    }

    if (a->qtdTokens >= MAX_TOKENS) {
        return TOKEN_NENHUM;
    }

    int id = a->qtdTokens;
    a->tokens[id].id = id;
    copiarTexto(a->tokens[id].expressao, sizeof(a->tokens[id].expressao), expressao);
    copiarTexto(a->tokens[id].rotulo, sizeof(a->tokens[id].rotulo), rotulo);
    a->qtdTokens++;
    return id;
}

const char *obterRotuloToken(const Automato *a, int tokenId) {
    if (a != NULL && tokenId >= 0 && tokenId < a->qtdTokens) {
        return a->tokens[tokenId].rotulo;
    }
    return "TOKEN";
}

const char *obterRotuloEstado(const Automato *a, int estado) {
    if (a == NULL || estado < 0 || estado >= a->qtdEstados) {
        return "X";
    }
    if (estado == a->estadoErro) {
        return "X";
    }
    return obterRotuloToken(a, a->estados[estado].tokenId);
}

void tiraEspaco(char *str) {
    if (str == NULL) {
        return;
    }

    size_t fim = strlen(str);
    while (fim > 0 && isspace((unsigned char)str[fim - 1])) {
        str[--fim] = '\0';
    }

    size_t inicio = 0;
    while (str[inicio] != '\0' && isspace((unsigned char)str[inicio])) {
        inicio++;
    }
    if (inicio > 0) {
        memmove(str, str + inicio, fim - inicio + 1);
    }
}

static void imprimirNomeEstado(const Automato *a, const Estado *estado) {
    if (estado->id == a->estadoInicial) {
        printf("->");
    }
    if (estado->id == a->estadoErro) {
        /* O número interno não faz parte da representação do AFD. */
        printf("X");
    } else if (estado->qtdSubconjunto > 0) {
        printf("{");
        for (int i = 0; i < estado->qtdSubconjunto; i++) {
            if (i > 0) {
                printf(",");
            }
            printf("%d", estado->subconjunto[i]);
        }
        printf("}");
    } else {
        printf("%d", estado->id);
    }
}

void imprimirTabela(const Automato *a, const char *titulo) {
    if (a == NULL) {
        return;
    }

    printf("\n---------- %s ----------\n\n", titulo != NULL ? titulo : "Autômato");
    printf("Estado\t|");
    for (int i = 0; i < a->qtdSimbolos; i++) {
        unsigned char simbolo = (unsigned char)a->alfabeto[i];
        if (isprint(simbolo)) {
            printf("\t%c\t|", simbolo);
        } else {
            printf("\t0x%02X\t|", simbolo);
        }
    }
    printf(" Final/Rótulo\n");
    printf("--------------------------------------------------------------------------------\n");

    for (int i = 0; i < a->qtdEstados; i++) {
        const Estado *estado = &a->estados[i];
        imprimirNomeEstado(a, estado);
        printf("\t|");

        for (int j = 0; j < a->qtdSimbolos; j++) {
            printf("\t");
            bool encontrou = false;
            for (int k = 0; k < estado->qtdTransicoes; k++) {
                if (estado->transicoes[k].simbolo == a->alfabeto[j]) {
                    if (encontrou) {
                        printf(",");
                    }
                    int destino = estado->transicoes[k].destino;
                    if (destino == a->estadoErro) {
                        printf("X");
                    } else if (a->estados[destino].qtdSubconjunto > 0) {
                        printf("{");
                        for (int m = 0; m < a->estados[destino].qtdSubconjunto; m++) {
                            if (m > 0) {
                                printf(",");
                            }
                            printf("%d", a->estados[destino].subconjunto[m]);
                        }
                        printf("}");
                    } else {
                        printf("%d", destino);
                    }
                    encontrou = true;
                }
            }
            if (!encontrou) {
                printf("-");
            }
            printf("\t|");
        }

        if (estado->id == a->estadoErro) {
            printf(" NÃO/ERRO\n");
        } else if (estado->final) {
            printf(" SIM/%s\n", obterRotuloEstado(a, estado->id));
        } else {
            printf(" NÃO\n");
        }
    }
}
