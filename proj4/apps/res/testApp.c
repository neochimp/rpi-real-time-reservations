#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/wait.h>

#include "dummy_task.h"

#ifndef __NR_set_rsv
#define __NR_set_rsv 397
#endif
#ifndef __NR_cancel_rsv
#define __NR_cancel_rsv 398
#endif
#ifndef __NR_wait_until_next_period
#define __NR_wait_until_next_period 399
#endif


struct threadTimes {
	struct timespec C;
	struct timespec T;
};

static long set_rsv(pid_t pid, const struct timespec *C, const struct timespec *T) {
    return syscall(__NR_set_rsv, pid, C, T);
}

static long cancel_rsv(pid_t pid){
	return syscall(__NR_cancel_rsv, pid);
}

static long wait_until_next_period(void){
	return syscall(__NR_wait_until_next_period);
}


int main(void) {

	long long Cvals[5] = {10e6, 100e6, 35e6, 50e6, 10e6};
	long long Tvals[5] = {50e6, 400e6, 200e6, 70e6, 10e6};
	
	calibrate();

	for(int i = 0; i < 5; i++){
		pid_t pid = fork();
		if(pid == 0){
			struct timespec C = {.tv_nsec=Cvals[i], .tv_sec = 0};
			struct timespec T = {.tv_nsec=Tvals[i], .tv_sec = 0};
		

			printf("Child PID %d: C=%ld T=%ld\n", getpid(), C.tv_nsec, T.tv_nsec);

			long long ret = set_rsv(getpid(), &C, &T);
			if(ret == -1){
				perror("set_rsv");
				exit(1);
			}
			long long C_ns = (long long)(C.tv_sec * 1e9 + C.tv_nsec);
			long long C_ms = C_ns / 1e6;
			
			long long T_ns = (long long)(T.tv_sec * 1e9 + T.tv_nsec);
			long long T_ms = T_ns / 1e6;
			while(1){
				struct timespec now;
				clock_gettime(CLOCK_MONOTONIC, &now);
				//printf("PID:%d t=%lld.%09ld Starting Task, Task: %lldms, Period: %lldms\n", getpid(), (long long)now.tv_sec, now.tv_nsec, C_ms, T_ms);
				dummy_load(C_ms);
				long wret = wait_until_next_period();
				if(wret < 0){
					printf("wait_until_next_period broke\n");
					exit(1);
				}
			}

			exit(0);
		}
	}
	for (int i = 0; i < 2; i++){
		wait(NULL);
	}


	return 0;
}



