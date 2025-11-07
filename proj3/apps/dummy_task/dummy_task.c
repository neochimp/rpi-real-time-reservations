/*
 * This function consumes CPU cycles for a duration determined by
 * the 'execution_time_ms' and the global 'dummy_load_calib' variable.
 */
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>


int dummy_load_calib = 1000;

void dummy_load(int execution_time_ms) {
    //struct timeval tv;
    int i, j;

    for (j = 0; j < dummy_load_calib * execution_time_ms; j++)
        for (i = 0; i < 1000; i++)
            __asm__ volatile ("nop");
}

void calibrate() {
    // target times might need adjusting, fixable later
    const int target = 100;
    const int its = 3;

    struct timeval tv;
    
    long long start_us, end_us;
    double total_time = 0.0;
    
    // stabilize CPU
    dummy_load(500);
    dummy_load(500);
    for (int i = 0; i < its; i++) { 
        // start timer
        gettimeofday(&tv, NULL);
        start_us = (long long)tv.tv_sec * 1000000LL + tv.tv_usec;
    
        dummy_load(target);
    
        // end timer
        gettimeofday(&tv, NULL);
        end_us = (long long)tv.tv_sec * 1000000LL + tv.tv_usec;
    
        total_time += (end_us - start_us) / 1000.0;
    }
    dummy_load_calib = (int)(((double)dummy_load_calib * target / (total_time / 3)) + 0.5);
}

int main(int argc, char* argv[]) {
	struct timeval tv;
    long long start, end;
    double elapsed_ms;

    int target_ms = atoi(argv[1]);

    calibrate();
    gettimeofday(&tv, NULL);
    start = (long long)tv.tv_sec * 1000000LL + tv.tv_usec;

	dummy_load(target_ms);

	gettimeofday(&tv, NULL);
	end = (long long)tv.tv_sec * 1000000LL + tv.tv_usec;

    elapsed_ms = (end - start) / 1000.0;
	printf("Took: %.3f ms (%.lld microseconds)\n", elapsed_ms, end-start);
}
