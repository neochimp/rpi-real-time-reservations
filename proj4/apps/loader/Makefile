#partially copied from proj4/apps/res/Makefile
CC := arm-linux-gnueabihf-gcc
CXX := arm-linux-gnueabihf-g++

CFLAGS += -pthread -I../dummy_task -Wall -g

SRCS := loader.c ../dummy_task/dummy_task.c
OBJS := $(SRCS:.c=.o)

all: loader

loader: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

loader.o: loader.h ../dummy_task/dummy_task.h
../dummy_task/dummy_task.o: ../dummy_task/dummy_task.h

clean:
	rm -f $(OBJS) loader
