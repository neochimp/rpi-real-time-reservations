#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>

/* System call numbers */
#define SET_RSV 397
#define CANCEL_RSV 398
#define WAIT_UNTIL_NEXT_PERIOD 399

/* Dummy load configuration */
#define DUMMY_LOAD_ITER 1000
#define TARGET_DURATION_MS 100
#define ERROR_MARGIN_US 500
#define MAX_CALIBRATION_ATTEMPTS 50
static int dummy_load_calib = 1;

/* Structure for task parameters */
struct set_task_param {
    struct timespec C;
    struct timespec T;
};

/* Test result tracking */
int test_passed = 0;
int test_failed = 0;

/* Dummy load function */
void dummy_load(int load_ms) {
    int i, j;
    for (j = 0; j < dummy_load_calib * load_ms; j++)
        for (i = 0; i < DUMMY_LOAD_ITER; i++)
            __asm__ volatile ("nop");
}

/* Helper function to calculate time difference in microseconds */
long get_time_diff_us(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
}

/* Calibrate dummy_load to ensure accurate timing */
int calibrate_dummy_load() {
    struct timeval start_time, end_time;
    long duration_us;
    int attempts = 0;
    int target_us = TARGET_DURATION_MS * 1000;

    printf("Starting dummy load calibration...\n");
    printf("Target duration: %d ms (%d us)\n", TARGET_DURATION_MS, target_us);

    while (attempts < MAX_CALIBRATION_ATTEMPTS) {
        gettimeofday(&start_time, NULL);
        dummy_load(TARGET_DURATION_MS);
        gettimeofday(&end_time, NULL);

        duration_us = get_time_diff_us(start_time, end_time);

        printf("Attempt %d: dummy_load_calib=%d, duration=%ld us, error=%ld us\n",
               attempts + 1, dummy_load_calib, duration_us, labs(duration_us - target_us));

        if (labs(duration_us - target_us) <= ERROR_MARGIN_US) {
            printf("Calibration successful! dummy_load_calib = %d\n", dummy_load_calib);
            return dummy_load_calib;
        }

        if (duration_us > 0) {
            dummy_load_calib = (target_us * dummy_load_calib) / duration_us;
            if (dummy_load_calib <= 0) dummy_load_calib = 1;
        } else {
            dummy_load_calib *= 2;
        }

        attempts++;
        usleep(10000); // 10ms delay between attempts
    }

    printf("Calibration failed after %d attempts\n", MAX_CALIBRATION_ATTEMPTS);
    printf("Using final parameter: dummy_load_calib = %d\n", dummy_load_calib);
    return -1;
}

void print_test_result(const char *test_name, int result, int expected) {
    if (result == expected) {
        printf("[PASS] %s\n", test_name);
        test_passed++;
    } else {
        printf("[FAIL] %s - Expected: %d, Got: %d (errno: %s)\n",
               test_name, expected, result, strerror(errno));
        test_failed++;
    }
}


/* Test 2: Invalid pointer test */
void test_invalid_pointers() {
    printf("\n=== Test 2: Invalid Pointer Handling ===\n");

    struct timespec C, T;
    C.tv_sec = 0;
    C.tv_nsec = 50000000;
    T.tv_sec = 0;
    T.tv_nsec = 100000000;
    pid_t pid = getpid();

    // NULL C pointer test
    int ret1 = syscall(SET_RSV, pid, NULL, &T);
    print_test_result("Set reservation with NULL C pointer", ret1, -1);

    // NULL T pointer test
    int ret2 = syscall(SET_RSV, pid, &C, NULL);
    print_test_result("Set reservation with NULL T pointer", ret2, -1);

    // Cancel non-existent reservation
    int ret3 = syscall(CANCEL_RSV, 99999);
    print_test_result("Cancel non-existent reservation", ret3, -1);
}

/* Test 1: Thread reservation with dummy load */
void *thread_test_func(void *arg) {
    struct set_task_param *params = (struct set_task_param *)arg;
    pid_t tid = syscall(SYS_gettid);
    int job_count = 0;
    struct sched_param sp;
    int policy;

    // Pin thread to CPU core 0
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
        printf("  [Thread %d] Warning: Failed to set CPU affinity\n", tid);
    }

    int c_ms = params->C.tv_sec * 1000 + params->C.tv_nsec / 1000000;
    int t_ms = params->T.tv_sec * 1000 + params->T.tv_nsec / 1000000;

    printf("  [Thread %d] C=%dms, T=%dms (pinned to CPU 0)\n", tid, c_ms, t_ms);

    // Set reservation for thread
    int ret = syscall(SET_RSV, tid, &params->C, &params->T);
    if (ret == 0) {
        // Get and print scheduling policy and RT priority
        if (pthread_getschedparam(pthread_self(), &policy, &sp) == 0) {
            const char *policy_name = (policy == SCHED_FIFO) ? "SCHED_FIFO" :
                                     (policy == SCHED_RR) ? "SCHED_RR" :
                                     (policy == SCHED_OTHER) ? "SCHED_OTHER" : "UNKNOWN";
            printf("  [Thread %d] Policy: %s, RT Priority: %d\n",
                   tid, policy_name, sp.sched_priority);
        } else {
            printf("  [Thread %d] Failed to get scheduling parameters\n", tid);
        }

        // Calculate load in milliseconds (90% of C), so no budget overrun will occur
        int load_ms = (c_ms * 90) / 100;

        // Execute periodic jobs
        while (job_count < 200) {
            // Execute computation
            dummy_load(load_ms);

            // Wait until next period
            int wait_ret = syscall(WAIT_UNTIL_NEXT_PERIOD);
            if (wait_ret < 0) {
                printf("  [Thread %d] Job %d: wait_until_next_period failed: %s\n",
                       tid, job_count, strerror(errno));
                break;
            }

            job_count++;
        }

        printf("  [Thread %d] Completed %d jobs\n", tid, job_count);

        // Cancel reservation
        syscall(CANCEL_RSV, tid);
        return (void *)0;
    } else {
        printf("  [Thread %d] Reservation failed: %s\n", tid, strerror(errno));
        return (void *)-1;
    }
}

/* Test 1: Single thread with budget overrun (110% of C) */
void *thread_overrun_func(void *arg) {
    struct set_task_param *params = (struct set_task_param *)arg;
    pid_t tid = syscall(SYS_gettid);
    int job_count = 0;
    struct sched_param sp;
    int policy;

    // Pin thread to CPU core 0
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
        printf("  [Thread %d] Warning: Failed to set CPU affinity\n", tid);
    }

    int c_ms = params->C.tv_sec * 1000 + params->C.tv_nsec / 1000000;
    int t_ms = params->T.tv_sec * 1000 + params->T.tv_nsec / 1000000;

    printf("  [Thread %d] C=%dms, T=%dms (pinned to CPU 0)\n", tid, c_ms, t_ms);

    // Set reservation for thread
    int ret = syscall(SET_RSV, tid, &params->C, &params->T);
    if (ret == 0) {
        // Get and print scheduling policy and RT priority
        if (pthread_getschedparam(pthread_self(), &policy, &sp) == 0) {
            const char *policy_name = (policy == SCHED_FIFO) ? "SCHED_FIFO" :
                                     (policy == SCHED_RR) ? "SCHED_RR" :
                                     (policy == SCHED_OTHER) ? "SCHED_OTHER" : "UNKNOWN";
            printf("  [Thread %d] Policy: %s, RT Priority: %d\n",
                   tid, policy_name, sp.sched_priority);
        }

        // Calculate load in milliseconds (110% of C) - intentional overrun
        int load_ms = (c_ms * 110) / 100;
        printf("  [Thread %d] Running with 110%% budget (load=%dms, C=%dms)\n",
               tid, load_ms, c_ms);

        // Execute periodic jobs
        while (job_count < 200) {
            printf("  [Thread %d] Job %d starting\n", tid, job_count);

            // Execute computation (will exceed budget)
            dummy_load(load_ms);

            // Wait until next period
            int wait_ret = syscall(WAIT_UNTIL_NEXT_PERIOD);
            if (wait_ret < 0) {
                printf("  [Thread %d] Job %d: wait_until_next_period failed: %s\n",
                       tid, job_count, strerror(errno));
                break;
            }

            job_count++;
        }

        printf("  [Thread %d] Completed %d jobs\n", tid, job_count);

        // Cancel reservation
        syscall(CANCEL_RSV, tid);
        return (void *)0;
    } else {
        printf("  [Thread %d] Reservation failed: %s\n", tid, strerror(errno));
        return (void *)-1;
    }
}

void test_budget_overrun() {
    printf("\n=== Test 1: Single Thread with Budget Overrun (110%% of C) ===\n");
    printf("Expected: Budget overrun messages in kernel log (dmesg)\n\n");

    pthread_t thread;
    struct set_task_param params;

    // Single thread with C=50ms, T=200ms
    params.C.tv_sec = 0;
    params.C.tv_nsec = 50000000;  // 50 ms
    params.T.tv_sec = 0;
    params.T.tv_nsec = 200000000; // 200 ms

    printf("Creating thread with C=50ms, T=200ms (will run at 110%% = 55ms)\n");

    int ret = pthread_create(&thread, NULL, thread_overrun_func, &params);
    if (ret == 0) {
        void *thread_ret;
        pthread_join(thread, &thread_ret);
        print_test_result("Single thread with budget overrun",
                         (long)thread_ret == 0 ? 0 : -1, 0);
    } else {
        print_test_result("Thread creation", -1, 0);
    }

    printf("\nCheck kernel log with 'dmesg' for budget overrun messages\n");
}

void test_thread_reservation() {
    printf("\n=== Test 2: Multiple Threads with Different Periods ===\n");
    printf("Expected: Rate Monotonic priority assignment (shorter T = higher priority)\n\n");

    #define NUM_THREADS 5
    pthread_t threads[NUM_THREADS];
    struct set_task_param params[NUM_THREADS];

    // Define different C and T values for each thread
    // Shuffled order to test Rate Monotonic priority assignment
    struct {
        int c_ms;
        int t_ms;
    } configs[NUM_THREADS] = {
        {30, 200},  // Thread 0: T=200ms
        {50, 500},  // Thread 1: T=500ms (should get lowest RT priority)
        {10, 50},   // Thread 2: T=50ms  (should get highest RT priority)
        {40, 300},  // Thread 3: T=300ms
        {20, 100}   // Thread 4: T=100ms
    };

    printf("Creating %d threads with different periods...\n", NUM_THREADS);

    // Create all threads
    for (int i = 0; i < NUM_THREADS; i++) {
        params[i].C.tv_sec = 0;
        params[i].C.tv_nsec = configs[i].c_ms * 1000000;
        params[i].T.tv_sec = 0;
        params[i].T.tv_nsec = configs[i].t_ms * 1000000;

        int ret = pthread_create(&threads[i], NULL, thread_test_func, &params[i]);
        if (ret != 0) {
            printf("Failed to create thread %d: %s\n", i, strerror(ret));
        }
        usleep(50000); // 50ms delay to ensure threads are created sequentially
    }

    // Wait for all threads to complete
    int success_count = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        void *thread_ret;
        pthread_join(threads[i], &thread_ret);
        if ((long)thread_ret == 0) {
            success_count++;
        }
    }

    printf("\n%d/%d threads completed successfully\n", success_count, NUM_THREADS);
    print_test_result("Multiple threads with RT priority assignment",
                     success_count == NUM_THREADS ? 0 : -1, 0);
}

static void usr1_handler(int signo, siginfo_t *info, void *ucontext){
	(void)ucontext;
	//printf("Got SIGUSR1 signo=%d, code=%d, pid=%d\n", signo, info->si_code, info->si_pid);
}

int main(int argc, char *argv[]) {
	
	struct sigaction sa = {0};
	sa.sa_sigaction = usr1_handler;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	
	if(sigaction(SIGUSR1, &sa, NULL) == -1){
		perror("sigaction");
		return 1;
	}




    printf("========================================\n");
    printf("  Reservation Management Test Suite\n");
    printf("========================================\n");
    printf("PID: %d\n\n", getpid());

    // Calibrate dummy_load for accurate timing
    if (calibrate_dummy_load() < 0) {
        printf("Warning: Calibration did not converge, using dummy_load_calib = %d\n", dummy_load_calib);
    }
    printf("\n");

    // Run all tests
    test_thread_reservation();
    test_invalid_pointers();
    test_budget_overrun();
    
    // Summary
    printf("\n========================================\n");
    printf("  Test Summary\n");
    printf("========================================\n");
    printf("Passed: %d\n", test_passed);
    printf("Failed: %d\n", test_failed);
    printf("Total:  %d\n", test_passed + test_failed);
    printf("========================================\n");

    return (test_failed == 0) ? 0 : 1;
}
