CC=gcc

build:
	$(CC) -o server lex/scan.c lex/token.c main.c

run:
	./server
