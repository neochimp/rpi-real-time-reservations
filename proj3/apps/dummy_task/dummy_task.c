/*
 * This function consumes CPU cycles for a duration determined by
 * the 'execution_time_ms' and the global 'dummy_load_calib' variable.
 */
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

#include "dummy_task.h"


void dummy_load(int execution_time_ms) {
    int i, j;
    struct timeval tv;
    long long start, end;
    double elapsed_ms;
	
    gettimeofday(&tv, NULL);
    start = (long long)tv.tv_sec * USEC_PER_SEC + tv.tv_usec;

    for (j = 0; j < dummy_load_calib * execution_time_ms; j++)
        for (i = 0; i < INNER_ITERATIONS; i++)
            __asm__ volatile ("nop");
    gettimeofday(&tv, NULL);
    end = (long long)tv.tv_sec * USEC_PER_SEC + tv.tv_usec;

    elapsed_ms = (end - start) / MSEC_PER_SEC;
    printf("Requested execution time: %d ms\n", execution_time_ms);
    printf("Actual execution time: %.3f ms\n", elapsed_ms);

}

void calibrate() {
    struct timeval tv;
    long long start_us, end_us;
    double total_time = 0.0;    
	printf("[*] DUMMY LOAD CALIBRATING\n");
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
    
        total_time += (double)(end_us - start_us) / MSEC_PER_SEC;
    }
    dummy_load_calib = (int)(((double)dummy_load_calib * CALIB_TARGET_MS / 
                       (total_time / CALIB_ITERATIONS)) + 0.5);
	printf("[*] DUMMY LOAD CALIBRATED\n");
}

//int main(int argc, char* argv[]) {
//	struct timeval tv;
//    long long start, end;
//    double elapsed_ms;
//    (void)argc;
//
//    int target_ms = atoi(argv[1]);
//
//    calibrate();
//    gettimeofday(&tv, NULL);
//    start = (long long)tv.tv_sec * USEC_PER_SEC + tv.tv_usec;
//	for(int i = 0; i < 100; i++){
//		dummy_load(target_ms);
//	}
//}
