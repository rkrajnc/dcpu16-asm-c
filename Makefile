CC=clang
CFLAGS=-g -O0
SHELL=zsh
DIFF=colordiff

main: assembler.o file.o label_table.o lexer.o operand.o parser.o statement.o token.o utils.o main.o
	$(CC) $(CFLAGS) -o main *.o

clean:
	rm -f *.o(.N) main

test: main
	$(SHELL) -c 'colordiff -u fixtures/expected.stdout <(./main)'
