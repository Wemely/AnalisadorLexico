CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99
OBJ = main.o automato.o processamento.o determinizacao.o lexico.o
TARGET = Projeto-Compiladores

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

%.o: %.c automato.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGET) fita.txt tabela_simbolos.txt

run: all
	./$(TARGET) entrada.txt sentenca.txt
