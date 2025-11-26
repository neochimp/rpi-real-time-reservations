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

	long Cvals[5] = {2, 1, 4, 5, 1};
	long Tvals[5] = {5, 4, 5, 7, 1};

	for(int i = 0; i < 5; i++){
		pid_t pid = fork();
		if(pid == 0){
			struct timespec C = {.tv_sec=0, .tv_nsec = Cvals[i]};
			struct timespec T = {.tv_sec=0, .tv_nsec = Tvals[i]};
		

			printf("Child PID %d: C=%ld T=%ld\n", getpid(), C.tv_nsec, T.tv_nsec);

			long ret = set_rsv(getpid(), &C, &T);
			if(ret == -1){
				perror("set_rsv");
				exit(1);
			}

			while(1){
		
			}

			exit(0);
		}
	}
	for (int i = 0; i < 5; i++){
		wait(NULL);
	}


	return 0;
}



