CC=gcc

build:
	$(CC) -o server -g lex/scan.c lex/token.c http/http.c server.c main.c

run:
	./server 8080

check:
	valgrind --leak-check=full ./server 8080
