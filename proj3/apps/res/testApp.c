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

#ifndef __NR_set_rsv
#define __NR_set_rsv 397
#endif
#ifndef __NR_cancel_rsv
#define __NR_cancel_rsv 398
#endif


struct threadTimes {
	struct timespec C;
	struct timespec T;
};

static long set_rsv(pid_t pid, const struct timespec *C, const struct timespec *T) {
    return syscall(__NR_set_rsv, pid, C, T);
}

void *taskThread(void *arg){
	struct threadTimes *tt = (struct threadTimes *)arg;

	printf("C: %ld T: %ld", tt->C.tv_nsec, tt->T.tv_nsec);
	long ret = set_rsv(getpid(), &tt->C, &tt->T);
	if(ret == -1){
		
        	fprintf(stderr, "set_rsv failed: %s (errno=%d)\n", strerror(errno), errno);
		return NULL;
	}
	printf("set_rsv okk\n");
	//while(1);

	return NULL;
}


int main(void) {

	struct threadTimes task1Struct;

    	task1Struct.C.tv_nsec = 1000;
	task1Struct.T.tv_nsec = 2000;
    	
	pthread_t task1;
	pthread_create(&task1, NULL, taskThread, &task1Struct);

	pthread_join(task1, NULL);
	return 0;
}



