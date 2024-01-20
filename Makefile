CFLAGS=-std=c11 -g -fno-common
CC=gcc
RVCC=$(RISCV)/bin/riscv64-unknown-linux-gnu-gcc

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)
TEST_SRCS=$(wildcard test/*.c)
TESTS=$(TEST_SRCS:.c=.exe)

thrvcc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJS): thrvcc.h

test/%.exe: thrvcc test/%.c
	$(RVCC) -o- -E -P -C test/$*.c | ./thrvcc -c -o test/$*.o -
	$(RVCC) -static -o $@ test/$*.o -xc test/common

test: $(TESTS)
	for i in $^; do echo $$i; $(RISCV)/bin/qemu-riscv64 -L $(RISCV)/sysroot ./$$i || exit 1; echo; done
	test/driver.sh

clean:
	rm -rf thrvcc tmp* $(TESTS) test/*.s test/*.exe
	find * -type f '(' -name '*~' -o -name '*.o' -o -name '*.s' ')' -exec rm {} ';'

.PHONY: clean test
