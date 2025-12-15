CC:=arm-linux-gnueabihf-gcc
CXX:=arm-linux-gnueabihf-g++

CFLAGS += -pthread -I../dummy_task

SRCS := testApp.c ../dummy_task/dummy_task.c
OBJS := $(SRCS:.c=.o)

all: testApp

testApp: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

../dummy_task/dummy_task.o: ../dummy_task/dummy_task.h

clean:
	rm -f $(OBJS) testApp
