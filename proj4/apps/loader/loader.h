#ifndef LOADER_H
#define LOADER_H

#include <time.h>
#include <sys/types.h>

#define NUM_CORES 2
#define NUM_TASKS 9

#ifndef __NR_set_edf_task
#define __NR_set_edf_task 397
#endif

#ifndef __NR_cancel_rsv
#define __NR_cancel_rsv 398
#endif

#ifndef __NR_wait_until_next_period
#define __NR_wait_until_next_period 399
#endif

typedef struct {
    int C_ms;
    int T_ms;
    int D_ms;
    int chain_id;
    int chain_pos;
    double util;
    int assigned_core;
    int id;
} Task;

// Convert milliseconds to timespec
struct timespec ms_to_timespec(int ms);

// Sorting helper
void swap_tasks(Task *task_one, Task *task_two);

// Bubble sort tasks in descending order of utilization
void sort_tasks(Task tasks[], int num_tasks);

#endif // LOADER_H
