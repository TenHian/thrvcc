CFLAGS=-std=c11 -g -fno-common

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

thrvcc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJS): thrvcc.h

test: thrvcc
	./test.sh
	./test-driver.sh

clean:
	rm -f thrvcc *.o *.s tmp* a.out

.PHONY: clean test
