# Projeto 1 — Reconhecedor léxico

Implementação em C do analisador léxico, o projeto constrói um AFND, determiniza-o, remove estados inalcançáveis/mortos, adiciona o estado de erro `X` e reconhece uma sentença com geração de FITA e Tabela de Símbolos.

## Compilação e execução

```bash
make clean
make
./Projeto-Compiladores entrada.txt sentenca.txt
```

Para reduzir a saída das tabelas dos autômatos:

```bash
./Projeto-Compiladores entrada.txt sentenca.txt --sem-tabelas
```

A execução grava `fita.txt` e `tabela_simbolos.txt`. O programa retorna código zero quando não há erro léxico e código diferente de zero quando há pelo menos um erro. O código ainda imprime todos os resultados para que a demonstração do projeto seja possível.

Também é possível construir o AFD sem analisar uma fonte:

```bash
./Projeto-Compiladores minha_especificacao.txt
```

O formato geral é:

```text
palavra_reservada
outra_palavra
<S> ::= a<A> | e<A>
<A> ::= a<A> | e<A> | ε
```

Linhas sem `<` no início são tokens literais. Assim, símbolos especiais podem ser definidos diretamente, por exemplo `+` ou `(`. Linhas vazias e linhas iniciadas por `#` são ignoradas. O primeiro estado da gramática é `S`; a primeira gramática regular é rotulada como `IDENTIFICADOR`.

## Arquivos principais

- `automato.h`: estruturas e interfaces do projeto.
- `automato.c`: estados, transições, alfabeto, rótulos e impressão das tabelas.
- `processamento.c`: leitura da especificação, tokens literais e gramática regular.
- `determinizacao.c`: construção de subconjuntos, remoção de estados e estado `X`.
- `lexico.c`: reconhecimento por maior lexema, FITA e Tabela de Símbolos.
- `main.c`: fluxo completo da execução.
- `entrada.txt`: especificação do exemplo da disciplina.
- `sentenca.txt`: sentença demonstrativa com um erro (`@`).