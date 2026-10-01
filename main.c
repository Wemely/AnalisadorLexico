/*
  Projeto 1 - Reconhecedor léxico
  GEX1223 - Construção de Compiladores - 2026/02
 
  Autores: Wemely Barreto e Ana Luiza Scatolon
  Matrículas: 20240008468, 20240012863
*/

/*
  Para rodar:
  make clean
  make 
  ./Projeto-Compiladores entrada.txt sentenca.txt
*/


#include "automato.h"

static bool arquivoExiste(const char *arquivo) {
    FILE *f = fopen(arquivo, "r");
    if (f == NULL) {
        return false;
    }
    fclose(f);
    return true;
}

static void imprimirUso(const char *programa) {
    printf("Uso: %s [especificacao.txt] [fonte.txt] [--sem-tabelas]\n", programa);
    printf("\nA especificação contém palavras reservadas e produções da gramática regular.\n");
    printf("A fonte é a sentença que será reconhecida pelo AFD.\n");
}

int main(int argc, char *argv[]) {
    const char *arquivoEspecificacao = "entrada.txt";
    const char *arquivoFonte = NULL;
    bool imprimirAutomatos = true;
    int posicional = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--ajuda") == 0 || strcmp(argv[i], "-h") == 0) {
            imprimirUso(argv[0]);
            return EXIT_SUCCESS;
        }
        if (strcmp(argv[i], "--sem-tabelas") == 0) {
            imprimirAutomatos = false;
            continue;
        }
        if (posicional == 0) {
            arquivoEspecificacao = argv[i];
        } else if (posicional == 1) {
            arquivoFonte = argv[i];
        } else {
            fprintf(stderr, "Erro: argumento inesperado '%s'.\n", argv[i]);
            imprimirUso(argv[0]);
            return EXIT_FAILURE;
        }
        posicional++;
    }

    /* Mantém uma execução demonstrável sem impedir o uso só para construir o AFD. */
    if (arquivoFonte == NULL && strcmp(arquivoEspecificacao, "entrada.txt") == 0 &&
        arquivoExiste("sentenca.txt")) {
        arquivoFonte = "sentenca.txt";
    }

    Automato afnd;
    Automato afd;
    iniciaAutomato(&afnd);

    printf("Compiladores - Especificação: %s\n", arquivoEspecificacao);
    if (!carregarArquivo(arquivoEspecificacao, &afnd)) {
        return EXIT_FAILURE;
    }

    if (imprimirAutomatos) {
        imprimirTabela(&afnd, "AFND inicial");
    }

    determinizar(&afnd, &afd);
    removerInalcancaveis(&afd);
    removerMortos(&afd);
    if (imprimirAutomatos) {
        imprimirTabela(&afd, "AFD determinizado, sem estados inalcançáveis e mortos");
    }

    adicionarEstadoErro(&afd);
    if (imprimirAutomatos) {
        imprimirTabela(&afd, "AFD completo com estado de erro X");
    }

    if (arquivoFonte == NULL) {
        printf("\nNenhuma fonte informada. Use: %s %s fonte.txt\n", argv[0], arquivoEspecificacao);
        return EXIT_SUCCESS;
    }

    AnaliseLexica analise;
    if (!analisarFonte(arquivoFonte, &afd, &analise)) {
        return EXIT_FAILURE;
    }
    imprimirAnalise(&analise);
    if (!salvarAnalise(&analise, "fita.txt", "tabela_simbolos.txt")) {
        return EXIT_FAILURE;
    }
    
    return analise.qtdErros == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
