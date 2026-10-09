#include "automato.h"

/*
 * Ordena os estados de um subconjunto para padronizar a representação do conjunto.
 * Isso permite que estados equivalentes sejam comparados de forma determinística.
 */
static void ordenarSubconjunto(int *sub, int qtd) {
    for (int i = 0; i < qtd - 1; i++) {
        for (int j = i + 1; j < qtd; j++) {
            if (sub[i] > sub[j]) {
                int temporario = sub[i];
                sub[i] = sub[j];
                sub[j] = temporario;
            }
        }
    }
}

/*
 * Compara dois subconjuntos para verificar se representam o mesmo conjunto de estados.
 * Essa checagem é o núcleo da determinização, porque cada novo estado do AFD depende de um conjunto único.
 */
static bool subconjuntosIguais(const int *s1, int q1, const int *s2, int q2) {
    if (q1 != q2) {
        return false;
    }
    for (int i = 0; i < q1; i++) {
        if (s1[i] != s2[i]) {
            return false;
        }
    }
    return true;
}

/*
 * Busca um estado do AFD cujo subconjunto já foi criado.
 * Se o mesmo conjunto aparecer novamente, ele é reaproveitado em vez de criar duplicatas.
 */
static int buscarEstadoPorSubconjunto(const Automato *afd, const int *sub, int qtd) {
    for (int i = 0; i < afd->qtdEstados; i++) {
        if (subconjuntosIguais(afd->estados[i].subconjunto,
                                afd->estados[i].qtdSubconjunto, sub, qtd)) {
            return i;
        }
    }
    return -1;
}

/*
 * Avalia um subconjunto do AFND para verificar se ele representa um estado final e qual token deve prevalecer.
 * O token com menor identificador tem prioridade, como definido no projeto.
 */
static int tokenDoSubconjunto(const Automato *afnd, const int *sub, int qtd,
                              bool *ehFinal) {
    int tokenId = TOKEN_NENHUM;
    *ehFinal = false;
    for (int i = 0; i < qtd; i++) {
        int id = sub[i];
        if (id < 0 || id >= afnd->qtdEstados) {
            continue;
        }
        const Estado *estado = &afnd->estados[id];
        if (estado->final) {
            *ehFinal = true;
            if (estado->tokenId != TOKEN_NENHUM &&
                (tokenId == TOKEN_NENHUM || estado->tokenId < tokenId)) {
                tokenId = estado->tokenId;
            }
        }
    }
    return tokenId;
}

/*
 * Determiniza o AFND, transformando cada conjunto de estados em um único estado do AFD.
 * A ideia é explorar todos os símbolos do alfabeto e montar o conjunto de destinos possíveis para cada estado atual.
 */
void determinizar(const Automato *afnd, Automato *afd) {
    iniciaAutomato(afd);
    if (afnd == NULL || afd == NULL || afnd->estadoInicial < 0 || afnd->qtdEstados == 0) {
        return;
    }

    // Copia o alfabeto e a tabela de tokens do AFND para manter a mesma linguagem no AFD.
    memcpy(afd->alfabeto, afnd->alfabeto, sizeof(afnd->alfabeto));
    afd->qtdSimbolos = afnd->qtdSimbolos;
    memcpy(afd->tokens, afnd->tokens, sizeof(afnd->tokens));
    afd->qtdTokens = afnd->qtdTokens;

    // O estado inicial do AFD representa o conjunto de estados alcançáveis imediatamente do AFND.
    int inicial = adicionarEstado(afd, afnd->estados[afnd->estadoInicial].final);
    if (inicial < 0) {
        return;
    }
    afd->estadoInicial = inicial;
    afd->estados[inicial].subconjunto[0] = afnd->estadoInicial;
    afd->estados[inicial].qtdSubconjunto = 1;
    afd->estados[inicial].tokenId = afnd->estados[afnd->estadoInicial].tokenId;

    // Para cada estado já criado, calcula o próximo subconjunto para cada símbolo do alfabeto.
    for (int processados = 0; processados < afd->qtdEstados; processados++) {
        Estado *atual = &afd->estados[processados];
        for (int i = 0; i < afd->qtdSimbolos; i++) {
            char simbolo = afd->alfabeto[i];
            int novoSub[MAX_ESTADOS];
            int qtdNovos = 0;

            // Coleta todos os destinos do AFND que podem ser alcançados com o mesmo símbolo.
            for (int j = 0; j < atual->qtdSubconjunto; j++) {
                int idAfnd = atual->subconjunto[j];
                if (idAfnd < 0 || idAfnd >= afnd->qtdEstados) {
                    continue;
                }
                const Estado *estadoAfnd = &afnd->estados[idAfnd];
                for (int k = 0; k < estadoAfnd->qtdTransicoes; k++) {
                    if (estadoAfnd->transicoes[k].simbolo != simbolo ||
                        qtdNovos >= MAX_ESTADOS) {
                        continue;
                    }
                    int destino = estadoAfnd->transicoes[k].destino;
                    bool existe = false;
                    for (int m = 0; m < qtdNovos; m++) {
                        if (novoSub[m] == destino) {
                            existe = true;
                            break;
                        }
                    }
                    if (!existe) {
                        novoSub[qtdNovos++] = destino;
                    }
                }
            }

            // Se não existem destinos possíveis, esse símbolo não gera uma transição do AFD.
            if (qtdNovos == 0) {
                continue;
            }
            ordenarSubconjunto(novoSub, qtdNovos);
            int destino = buscarEstadoPorSubconjunto(afd, novoSub, qtdNovos);
            if (destino < 0) {
                bool ehFinal;
                int tokenId = tokenDoSubconjunto(afnd, novoSub, qtdNovos, &ehFinal);
                destino = adicionarEstado(afd, ehFinal);
                if (destino < 0) {
                    continue;
                }
                afd->estados[destino].qtdSubconjunto = qtdNovos;
                memcpy(afd->estados[destino].subconjunto, novoSub,
                       sizeof(int) * (size_t)qtdNovos);
                afd->estados[destino].tokenId = tokenId;
            }
            adicionarTransicao(afd, processados, destino, simbolo);
        }
    }
}

/*
 * Copia metadados do estado original para o novo estado da versão simplificada.
 * Isso preserva informações como token, estado final e subconjunto sem duplicar lógica.
 */
static void copiarMetadadosEstado(const Estado *origem, Estado *destino) {
    destino->final = origem->final;
    destino->tokenId = origem->tokenId;
    destino->qtdSubconjunto = origem->qtdSubconjunto;
    memcpy(destino->subconjunto, origem->subconjunto,
           sizeof(int) * (size_t)origem->qtdSubconjunto);
}

/*
 * Remove estados que não são alcançáveis a partir do estado inicial.
 * Essa limpeza reduz o tamanho do autômato sem afetar a linguagem reconhecida.
 */
void removerInalcancaveis(Automato *a) {
    if (a == NULL || a->qtdEstados == 0 || a->estadoInicial < 0) {
        return;
    }

    // BFS para descobrir quais estados são alcançáveis a partir do estado inicial.
    bool alcancaveis[MAX_ESTADOS] = {false};
    int fila[MAX_ESTADOS];
    int frente = 0;
    int tras = 0;
    fila[tras++] = a->estadoInicial;
    alcancaveis[a->estadoInicial] = true;

    while (frente < tras) {
        int atual = fila[frente++];
        for (int i = 0; i < a->estados[atual].qtdTransicoes; i++) {
            int destino = a->estados[atual].transicoes[i].destino;
            if (destino >= 0 && destino < a->qtdEstados && !alcancaveis[destino]) {
                alcancaveis[destino] = true;
                fila[tras++] = destino;
            }
        }
    }

    // Copia apenas os estados que permanecem acessíveis para formar uma versão reduzida do AFD.
    int novoId[MAX_ESTADOS];
    for (int i = 0; i < MAX_ESTADOS; i++) {
        novoId[i] = -1;
    }
    Automato novo;
    iniciaAutomato(&novo);
    memcpy(novo.alfabeto, a->alfabeto, sizeof(a->alfabeto));
    novo.qtdSimbolos = a->qtdSimbolos;
    memcpy(novo.tokens, a->tokens, sizeof(a->tokens));
    novo.qtdTokens = a->qtdTokens;

    for (int i = 0; i < a->qtdEstados; i++) {
        if (alcancaveis[i]) {
            novoId[i] = adicionarEstado(&novo, a->estados[i].final);
            copiarMetadadosEstado(&a->estados[i], &novo.estados[novoId[i]]);
        }
    }
    novo.estadoInicial = novoId[a->estadoInicial];
    if (a->estadoErro >= 0 && a->estadoErro < a->qtdEstados) {
        novo.estadoErro = novoId[a->estadoErro];
    }

    for (int i = 0; i < a->qtdEstados; i++) {
        if (!alcancaveis[i]) {
            continue;
        }
        for (int j = 0; j < a->estados[i].qtdTransicoes; j++) {
            int destino = a->estados[i].transicoes[j].destino;
            if (destino >= 0 && destino < a->qtdEstados && novoId[destino] >= 0) {
                adicionarTransicao(&novo, novoId[i], novoId[destino],
                                   a->estados[i].transicoes[j].simbolo);
            }
        }
    }
    *a = novo;
}

/*
 * Remove estados mortos, ou seja, estados que não levam a um estado final.
 * Isso simplifica a máquina e evita caminhos inúteis na análise léxica.
 */
void removerMortos(Automato *a) {
    if (a == NULL || a->qtdEstados == 0 || a->estadoInicial < 0) {
        return;
    }

    // Marca como vivos os estados que alcançam algum final e percorre o grafo de trás para frente.
    bool vivos[MAX_ESTADOS] = {false};
    int fila[MAX_ESTADOS];
    int frente = 0;
    int tras = 0;

    for (int i = 0; i < a->qtdEstados; i++) {
        if (a->estados[i].final) {
            vivos[i] = true;
            fila[tras++] = i;
        }
    }

    while (frente < tras) {
        int atual = fila[frente++];
        for (int i = 0; i < a->qtdEstados; i++) {
            if (vivos[i]) {
                continue;
            }
            for (int j = 0; j < a->estados[i].qtdTransicoes; j++) {
                if (a->estados[i].transicoes[j].destino == atual) {
                    vivos[i] = true;
                    fila[tras++] = i;
                    break;
                }
            }
        }
    }

    /* Mesmo sem caminho para um final, o estado inicial deve permanecer válido. */
    vivos[a->estadoInicial] = true;

    // Mantém somente os estados vivos para reduzir o autômato sem perder a linguagem.
    int novoId[MAX_ESTADOS];
    for (int i = 0; i < MAX_ESTADOS; i++) {
        novoId[i] = -1;
    }
    Automato novo;
    iniciaAutomato(&novo);
    memcpy(novo.alfabeto, a->alfabeto, sizeof(a->alfabeto));
    novo.qtdSimbolos = a->qtdSimbolos;
    memcpy(novo.tokens, a->tokens, sizeof(a->tokens));
    novo.qtdTokens = a->qtdTokens;

    for (int i = 0; i < a->qtdEstados; i++) {
        if (vivos[i]) {
            novoId[i] = adicionarEstado(&novo, a->estados[i].final);
            copiarMetadadosEstado(&a->estados[i], &novo.estados[novoId[i]]);
        }
    }
    novo.estadoInicial = novoId[a->estadoInicial];
    if (a->estadoErro >= 0 && a->estadoErro < a->qtdEstados) {
        novo.estadoErro = novoId[a->estadoErro];
    }

    for (int i = 0; i < a->qtdEstados; i++) {
        if (!vivos[i]) {
            continue;
        }
        for (int j = 0; j < a->estados[i].qtdTransicoes; j++) {
            int destino = a->estados[i].transicoes[j].destino;
            if (destino >= 0 && destino < a->qtdEstados && novoId[destino] >= 0) {
                adicionarTransicao(&novo, novoId[i], novoId[destino],
                                   a->estados[i].transicoes[j].simbolo);
            }
        }
    }
    *a = novo;
}

/*
 * Cria um estado especial que representa qualquer transição indefinida.
 * Ele redireciona entradas inválidas para um único ponto de erro, facilitando o reconhecimento léxico.
 */
void adicionarEstadoErro(Automato *a) {
    if (a == NULL || a->estadoErro >= 0) {
        return;
    }

    // Cria o estado especial que representa qualquer entrada indefinida no autômato.
    int idErro = adicionarEstado(a, false);
    if (idErro < 0) {
        return;
    }
    a->estadoErro = idErro;

    // O estado de erro permanece em laço para qualquer símbolo do alfabeto.
    for (int i = 0; i < a->qtdSimbolos; i++) {
        adicionarTransicao(a, idErro, idErro, a->alfabeto[i]);
    }

    // Redireciona cada transição inexistente para o estado de erro para tornar o AFD completo.
    int qtdOriginal = a->qtdEstados - 1;
    for (int i = 0; i < qtdOriginal; i++) {
        for (int j = 0; j < a->qtdSimbolos; j++) {
            char simbolo = a->alfabeto[j];
            if (buscarTransicao(a, i, simbolo) < 0) {
                adicionarTransicao(a, i, idErro, simbolo);
            }
        }
    }
}
