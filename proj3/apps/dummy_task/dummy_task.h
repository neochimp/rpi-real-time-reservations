#ifndef DUMMY_TASK_H
#define DUMMY_TASK_H

#define USEC_PER_SEC 1000000LL
#define MSEC_PER_SEC 1000.0

#define INNER_ITERATIONS 1000 // how many times our "nop" instruction is ran
#define DEFAULT_CALIB    200 // initial guess for calib

#define CALIB_WARMUP_MS   500 // each CPU warmup burst
#define CALIB_WARMUP_RUNS 2   // how many warmups
#define CALIB_TARGET_MS   100 // each measured run target
#define CALIB_ITERATIONS  10  // how many measured runs

// global calibration variable
static int dummy_load_calib = DEFAULT_CALIB;
extern int dummy_load_calib;

// consumes CPU cycles for a given parameter of milliseconds
void dummy_load(int);

// calibrates dummy_load_calib variable to ensure timing accuracy across
// CPUs
void calibrate();

#endif
