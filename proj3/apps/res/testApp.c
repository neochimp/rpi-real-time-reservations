#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#ifndef __NR_set_rsv
#define __NR_set_rsv 397
#endif
#ifndef __NR_cancel_rsv
#define __NR_cancel_rsv 398
#endif

static long set_rsv(pid_t pid, const struct timespec *C, const struct timespec *T) {
    return syscall(__NR_set_rsv, pid, C, T);
}

int main(void) {
    pid_t pid = getpid();

    struct timespec C, T;
    memset(&C, 0, sizeof(C));
    memset(&T, 0, sizeof(T));
    // Example values; adjust to what your kernel expects:
    C.tv_sec  = 0;     C.tv_nsec = 1000;
    T.tv_sec  = 0;     T.tv_nsec = 2000;

    long ret = set_rsv(pid, &C, &T);
    if (ret == -1) {
        fprintf(stderr, "set_rsv failed: %s (errno=%d)\n", strerror(errno), errno);
        return 1;
    }
    printf("set_rsv ok: ret=%ld\n", ret);
    return 0;
}

