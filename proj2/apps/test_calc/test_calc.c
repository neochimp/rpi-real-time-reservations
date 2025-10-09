#define _GNU_SOURCE //For system calls
#include <stdio.h> //io
#include <stdlib.h> 
#include <unistd.h> //syscalls
#include <sys/syscall.h> //syscalls
#include <errno.h> //for  errno
#include <limits.h> //for min/max

#ifndef __NR_calc
#define __NR_calc 397
#endif

int main(int argc, char **argv) {
	if(argc != 4) { //If not 4 arguments return error
		printf("Incorrect number of arguments");
		return 1;
	}
	
	//Convert to int (have to use strtol because atoi doesn't handle errors well)
	char *endptr1;
	char *endptr2;
	errno = 0;

	long firstInt = strtol(argv[1], &endptr1, 10);
	long secondInt = strtol(argv[3], &endptr2, 10);
	
	//Checks to see if integer arguments were correct
	if((argv[1] == endptr1) || (argv[3] == endptr2)) {
		printf("NaN\n");
		return 0;
	}
	if(errno == ERANGE) {
		printf("NaN\n");
		return 0;
	}
	if((*endptr1 != '\0') || (*endptr2 != '\0')) {
		printf("NaN\n");	
		return 0;
	}
	if((firstInt > INT_MAX) || (firstInt < INT_MIN) || (secondInt > INT_MAX) || (secondInt < INT_MIN)) {
		printf("NaN\n");
		return 0;
	}
	
	char op = argv[2][0]; //
	int result = 0;
	
	//System call to do math (and evaluate return code)
	long rc = syscall(__NR_calc, (int)firstInt, (int)secondInt, op, &result);
	if(rc == 0) { //worked
		printf("%d\n", result);
	}
	else {
		printf("NaN\n");
	}
	return 0;
}
