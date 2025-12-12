CC = arm-linux-gnueabihf-gcc
CXX:=arm-linux-gnueabihf-g++
CFLAGS = -Wall -Wextra -g -std=c99
LDFLAGS = -lpthread

all: test_rsv_simple

test_rsv_simple: test_rsv_simple.c
	$(CC) $(CFLAGS) -o test_rsv_simple test_rsv_simple.c $(LDFLAGS)

clean:
	rm -f test_rsv_simple

.PHONY: all clean
