#include "automato.h"

/*
 * Copia uma string de origem para destino de forma segura.
 * Como os textos dos tokens e rótulos têm tamanho limitado, essa rotina protege contra overflow.
 */
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

/*
 * Inicializa a estrutura do autômato limpando todos os campos.
 * Isso garante que estados, transições e alfabeto comecem em um estado consistente.
 */
void iniciaAutomato(Automato *a) {
    if (a == NULL) {
        return;
    }
    // Limpa todos os campos para que o autômato comece em um estado consistente.
    memset(a, 0, sizeof(*a));
    a->estadoInicial = -1;
    a->estadoErro = -1;
}

/*
 * Cria um novo estado no autômato e o marca como final quando solicitado.
 * O primeiro estado criado também vira o inicial, pois o projeto trabalha com uma máquina única.
 */
int adicionarEstado(Automato *a, bool ehFinal) {
    if (a == NULL || a->qtdEstados >= MAX_ESTADOS) {
        return -1;
    }

    // Usa o próximo índice disponível como identificador do estado novo.
    int id = a->qtdEstados++;
    Estado *estado = &a->estados[id];
    memset(estado, 0, sizeof(*estado));
    estado->id = id;
    estado->final = ehFinal;
    estado->tokenId = TOKEN_NENHUM;

    // O primeiro estado criado vira origem da máquina, pois o projeto usa uma máquina única.
    if (a->estadoInicial < 0) {
        a->estadoInicial = id;
    }
    return id;
}

/*
 * Associa um estado com um token reconhecido.
 * A prioridade menor significa que o token declarado primeiro no arquivo tem precedência.
 */
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

/*
 * Adiciona uma transição de um estado para outro sob um símbolo específico.
 * A lista de transições do estado cresce e o símbolo também entra no alfabeto do autômato.
 */
void adicionarTransicao(Automato *a, int origem, int destino, char simbolo) {
    if (a == NULL || origem < 0 || origem >= a->qtdEstados ||
        destino < 0 || destino >= a->qtdEstados || simbolo == '\0') {
        return;
    }

    // Registra a ligação entre o estado de origem e o próximo estado sob um símbolo específico.
    Estado *estado = &a->estados[origem];
    if (estado->qtdTransicoes >= MAX_TRANSICOES) {
        return;
    }

    estado->transicoes[estado->qtdTransicoes].simbolo = simbolo;
    estado->transicoes[estado->qtdTransicoes].destino = destino;
    estado->qtdTransicoes++;

    // O símbolo também entra no alfabeto do autômato para imprimir a tabela corretamente.
    adicionarSimboloAlfabeto(a, simbolo);
}

/*
 * Guarda os símbolos que aparecem no autômato para imprimir a tabela de transições.
 * Isso evita duplicar letras no alfabeto e ajuda a construir colunas da tabela.
 */
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

/*
 * Busca a transição disponível a partir do estado atual para um símbolo específico.
 * Se não existir, retorna -1, indicando que a máquina não aceita esse símbolo nesse ponto.
 */
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

/*
 * Registra um token novo no autômato, preservando expressão e rótulo.
 * Essa estrutura é usada depois para converter o lexema reconhecido em um nome semântico.
 */
int registrarToken(Automato *a, const char *expressao, const char *rotulo) {
    if (a == NULL || expressao == NULL || rotulo == NULL || expressao[0] == '\0' ||
        rotulo[0] == '\0') {
        return TOKEN_NENHUM;
    }

    // Evita duplicar um token já registrado no mesmo formato e mesmo rótulo.
    for (int i = 0; i < a->qtdTokens; i++) {
        if (strcmp(a->tokens[i].expressao, expressao) == 0 &&
            strcmp(a->tokens[i].rotulo, rotulo) == 0) {
            return a->tokens[i].id;
        }
    }

    if (a->qtdTokens >= MAX_TOKENS) {
        return TOKEN_NENHUM;
    }

    // Cria um novo token com o identificador sequencial e armazena o texto/descrição.
    int id = a->qtdTokens;
    a->tokens[id].id = id;
    copiarTexto(a->tokens[id].expressao, sizeof(a->tokens[id].expressao), expressao);
    copiarTexto(a->tokens[id].rotulo, sizeof(a->tokens[id].rotulo), rotulo);
    a->qtdTokens++;
    return id;
}

/*
 * Retorna o nome legível associado a um identificador de token.
 * Quando o token não existe, o programa usa um rótulo genérico para evitar falhas.
 */
const char *obterRotuloToken(const Automato *a, int tokenId) {
    if (a != NULL && tokenId >= 0 && tokenId < a->qtdTokens) {
        return a->tokens[tokenId].rotulo;
    }
    return "TOKEN";
}

/*
 * Consulta o rótulo do estado, retornando 'X' caso seja o de erro ou estado inválido.
 * Essa função ajuda a decidir se a tabela deve mostrar um estado final ou um erro.
 */
const char *obterRotuloEstado(const Automato *a, int estado) {
    if (a == NULL || estado < 0 || estado >= a->qtdEstados) {
        return "X";
    }
    if (estado == a->estadoErro) {
        return "X";
    }
    return obterRotuloToken(a, a->estados[estado].tokenId);
}

/*
 * Remove espaços em branco no início e no fim da string.
 * Essa rotina facilita a leitura de regras e tokens no arquivo de especificação.
 */
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

/*
 * Formata a representação textual do estado para a tabela do autômato.
 * Estados iniciais recebem uma seta, e estados de erro são mostrados como X.
 */
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

/*
 * Imprime a tabela do autômato com colunas para cada símbolo do alfabeto.
 * Essa visão facilita a depuração e a validação da construção do AFD/AFND.
 */
void imprimirTabela(const Automato *a, const char *titulo) {
    if (a == NULL) {
        return;
    }

    // Cabeçalho da tabela para identificar a etapa da construção do autômato.
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

    // Para cada estado, imprime o destino para cada símbolo do alfabeto e se ele é final.
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
