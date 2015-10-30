CC=gcc

all: main.c
	$(CC) -pthread main.c -o matMultp
clean:
	rm matMultp
