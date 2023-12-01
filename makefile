CC=gcc

build:
	$(CC) -o server -g lex/scan.c lex/token.c main.c

run:
	./server
