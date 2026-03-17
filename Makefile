CC=gcc
CFLAGS=-g3 -O0 -lpthread

all: 0_test_accumulate

%: %.c
	$(CC) $< tp.c -o $@

clean:
	rm -f *.o 0_test_accumulate
