#ifndef AUTOMATO_H
#define AUTOMATO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_ESTADOS 500
#define MAX_TRANSICOES 128
#define MAX_SIMBOLOS 128
#define MAX_LINHA 512
#define MAX_TOKENS 128
#define MAX_LEXEMA 256
#define MAX_ROTULO 64
#define MAX_REGISTROS 4096
#define TOKEN_NENHUM (-1)

typedef struct {
    char simbolo;
    int destino;
} Transicao;

typedef struct {
    int id;
    bool final;
    int tokenId;
    int qtdTransicoes;
    Transicao transicoes[MAX_TRANSICOES];
    int subconjunto[MAX_ESTADOS];
    int qtdSubconjunto;
} Estado;

typedef struct {
    int id;
    char rotulo[MAX_ROTULO];
    char expressao[MAX_LEXEMA];
} TokenDef;

typedef struct {
    Estado estados[MAX_ESTADOS];
    int qtdEstados;
    int estadoInicial;
    int estadoErro;
    char alfabeto[MAX_SIMBOLOS];
    int qtdSimbolos;
    TokenDef tokens[MAX_TOKENS];
    int qtdTokens;
} Automato;

typedef struct {
    int linha;
    char identificador[MAX_LEXEMA];
    char rotulo[MAX_ROTULO];
    int tokenId;
    bool erro;
} RegistroSimbolo;

typedef struct {
    RegistroSimbolo registros[MAX_REGISTROS];
    size_t qtdRegistros;
    char fita[MAX_REGISTROS][MAX_ROTULO];
    size_t qtdFita;
    size_t qtdErros;
} AnaliseLexica;

/* Manipulação do autômato. */
void iniciaAutomato(Automato *a);
int adicionarEstado(Automato *a, bool ehFinal);
void definirEstadoToken(Automato *a, int estado, int tokenId);
void adicionarTransicao(Automato *a, int origem, int destino, char simbolo);
void adicionarSimboloAlfabeto(Automato *a, char simbolo);
int buscarTransicao(const Automato *a, int estado, char simbolo);
int registrarToken(Automato *a, const char *expressao, const char *rotulo);
const char *obterRotuloToken(const Automato *a, int tokenId);
const char *obterRotuloEstado(const Automato *a, int estado);
void imprimirTabela(const Automato *a, const char *titulo);

/* Construção do AFND a partir do arquivo de especificação. */
bool carregarArquivo(const char *arquivo, Automato *afnd);
void processarToken(char *token, Automato *afnd);
void processarGramatica(char *linha, Automato *afnd);

/* Determinização e simplificação do AFD. */
void determinizar(const Automato *afnd, Automato *afd);
void removerInalcancaveis(Automato *a);
void removerMortos(Automato *a);
void adicionarEstadoErro(Automato *a);

/* Reconhecimento léxico. */
void inicializarAnalise(AnaliseLexica *analise);
bool analisarFonte(const char *arquivo, const Automato *afd, AnaliseLexica *analise);
void imprimirAnalise(const AnaliseLexica *analise);
bool salvarAnalise(const AnaliseLexica *analise, const char *arquivoFita,const char *arquivoTabela);

void tiraEspaco(char *str);

#endif
