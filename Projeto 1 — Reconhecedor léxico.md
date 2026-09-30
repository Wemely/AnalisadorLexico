# Projeto 1 — Reconhecedor léxico

Implementação em C do analisador léxico solicitado no PDF da disciplina. O projeto constrói um AFND a partir da especificação, determiniza-o, remove estados inalcançáveis/mortos, adiciona o estado de erro `X` e reconhece uma sentença com geração de FITA e Tabela de Símbolos.

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
- `artigo.md`: artigo solicitado no enunciado, em seções corridas.
- `artigo_sbc.tex`: fonte LaTeX preparada para o template SBC.
- `sbc-template.sty`: estilo SBC que acompanha a fonte LaTeX.
- `artigo.pdf`: versão PDF de quatro páginas para leitura/entrega.

Se houver uma distribuição LaTeX instalada, a fonte SBC pode ser compilada com:

```bash
pdflatex artigo_sbc.tex
```

O PDF `artigo.pdf` já acompanha o projeto porque o ambiente de execução não possui
`pdflatex`; no Overleaf, basta enviar `artigo_sbc.tex` e `sbc-template.sty`.
