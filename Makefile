
CC=gcc
CFLAGS=-ansi -pedantic -Wall -Og

all: lz77x

clean:
	rm -f lz77x

lz77x: lz77x.c
	$(CC) $(CFLAGS) lz77x.c -o lz77x
