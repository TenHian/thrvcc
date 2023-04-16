CFLAGS=-std=c11 -g -fno-common

thrvcc: main.o
	$(CC) -o thrvcc $(CFLAGS) main.o

test: thrvcc
	./test.sh

clean:
	rm -f thrvcc *.o *.s tmp* a.out

.PHONY: clean test
