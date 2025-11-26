CC:=arm-linux-gnueabihf-gcc
CXX:=arm-linux-gnueabihf-g++

CFLAGS += -pthread -I../dummy_task

SRCS := testApp.c ../dummy_task/dummy_task.c
OBJS := $(SRCS:.c=.c)

all: testApp

testApp: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
	
