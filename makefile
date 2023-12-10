CC=gcc

build:
	$(CC) -o server -g lex/scan.c lex/token.c hash/hashtab.c server.c main.c

run:
	./server 8080
