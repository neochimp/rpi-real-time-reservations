/*
 * This function consumes CPU cycles for a duration determined by
 * the 'execution_time_ms' and the global 'dummy_load_calib' variable.
 */
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

#define USEC_PER_SEC 1000000LL
#define MSEC_PER_SEC 1000.0

#define INNER_ITERATIONS 1000 // how many times our "nop" instruction is ran
#define DEFAULT_CALIB    1000 // initial guess for calib

#define CALIB_WARMUP_MS   500 // each CPU warmup burst
#define CALIB_WARMUP_RUNS 2   // how many warmups
#define CALIB_TARGET_MS   100 // each measured run target
#define CALIB_ITERATIONS  10  // how many measured runs


// global calibration var
int dummy_load_calib = DEFAULT_CALIB;

void dummy_load(int execution_time_ms) {
    int i, j;

    for (j = 0; j < dummy_load_calib * execution_time_ms; j++)
        for (i = 0; i < INNER_ITERATIONS; i++)
            __asm__ volatile ("nop");
}

void calibrate() {
    struct timeval tv;
    long long start_us, end_us;
    double total_time = 0.0;
    
    // stabilize CPU
    for (int i = 0; i < CALIB_WARMUP_RUNS; i++) {
        dummy_load(CALIB_WARMUP_MS);
    }
    
    for (int i = 0; i < CALIB_ITERATIONS; i++) { 
        // start timer
        gettimeofday(&tv, NULL);
        start_us = (long long)tv.tv_sec * USEC_PER_SEC + tv.tv_usec;
    
        dummy_load(CALIB_TARGET_MS);
    
        // end timer
        gettimeofday(&tv, NULL);
        end_us = (long long)tv.tv_sec * USEC_PER_SEC + tv.tv_usec;
    
        total_time += (end_us - start_us) / MSEC_PER_SEC;
    }
    dummy_load_calib = (int)(((double)dummy_load_calib * CALIB_TARGET_MS / 
                       (total_time / CALIB_ITERATIONS)) + 0.5);
}

int main(int argc, char* argv[]) {
	struct timeval tv;
    long long start, end;
    double elapsed_ms;

    int target_ms = atoi(argv[1]);

    calibrate();
    gettimeofday(&tv, NULL);
    start = (long long)tv.tv_sec * USEC_PER_SEC + tv.tv_usec;

	dummy_load(target_ms);

	gettimeofday(&tv, NULL);
	end = (long long)tv.tv_sec * USEC_PER_SEC + tv.tv_usec;

    elapsed_ms = (end - start) / MSEC_PER_SEC;
    printf("Requested execution time: %.3f ms\n", (double)target_ms);
	printf("Actual execution time: %.3f ms\n", elapsed_ms, end-start);
}
