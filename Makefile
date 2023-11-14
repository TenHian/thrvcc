CFLAGS=-std=c11 -g -fno-common
CC=gcc
RVCC=riscv64-elf-gcc

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)
TEST_SRCS=$(wildcard test/*.c)
TESTS=$(TEST_SRCS:.c=.exe)

thrvcc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJS): thrvcc.h

test/%.exe: thrvcc test/%.c
	$(RVCC) -o- -E -P -C test/$*.c | ./thrvcc -o test/$*.o -
	$(RVCC) -static -o $@ test/$*.o -xc test/common

test: $(TESTS)
	for i in $^; do echo $$i; qemu-riscv64 ./$$i || exit 1; echo; done
	test/driver.sh

clean:
	rm -rf thrvcc tmp* $(TESTS) test/*.s test/*.exe
	find * -type f '(' -name '*~' -o -name '*.o' -o -name '*.s' ')' -exec rm {} ';'

.PHONY: clean test
