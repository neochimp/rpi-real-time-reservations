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
    const int target_test1 = 50;
    const int target_test2 = 100;

    struct timeval tv;
    
    long long start_us, end_us;
    double actual_time1, actual_time2;
    
    // stabilize CPU
    dummy_load(500);
    dummy_load(500);
   
    // start timer
    gettimeofday(&tv, NULL);
    start_us = (long long)tv.tv_sec * 1000000LL + tv.tv_usec;
    
    dummy_load(target_test1);
    
    // end timer
    gettimeofday(&tv, NULL);
    end_us = (long long)tv.tv_sec * 1000000LL + tv.tv_usec;
    
    // find actual time in milliseconds
    actual_time1 = (end_us - start_us) / 1000.0;

    gettimeofday(&tv, NULL);
    start_us = (long long)tv.tv_sec * 1000000LL + tv.tv_usec;
    
    dummy_load(target_test2);
    
    gettimeofday(&tv, NULL);
    end_us = (long long)tv.tv_sec * 1000000LL + tv.tv_usec;
    
    actual_time2 = (end_us - start_us) / 1000.0;
    
    // find slope of ms per iteration-unit
    double slope = (actual_time2 - actual_time1) / 
                   (dummy_load_calib * (target_test2 - target_test1));
    
    // inverse calibration (gives us how many loop iterations per millisecond are needed)
    dummy_load_calib = (int)((1./slope)+0.5);
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
