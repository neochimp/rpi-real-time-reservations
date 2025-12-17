#define _GNU_SOURCE // Required for pthread_setaffinity_np
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //for fork()
#include <sched.h> //for cpu_set_t
#include <time.h> //for timespec
#include <sys/types.h> //for pid_t
#include <sys/syscall.h> //for calling syscalls
#include <sys/wait.h>
#include <signal.h> //for kill()?

#include "../dummy_task/dummy_task.h"
#include "loader.h"

struct timespec ms_to_timespec(int ms) {
    struct timespec temp;
    temp.tv_sec = ms / 1000;
    temp.tv_nsec = (ms % 1000) * 1000000;
    return temp;
}

static long set_edf_task(pid_t pid,
                         const struct timespec *C,
                         const struct timespec *T,
                         const struct timespec *D,
                         int cpu_id,
                         int chain_id,
                         int chain_pos){
    return syscall(__NR_set_edf_task, pid, C, T, D, cpu_id, chain_id, chain_pos);
}

static long cancel_rsv(pid_t pid) {
    return syscall(__NR_cancel_rsv, pid);
}

static long wait_until_next_period(void) {
    return syscall(__NR_wait_until_next_period);
}

void compare_task_util(const Task *task_one, const Task *task_two) {
    if (task_one->util < task_two->util) return 1;
    if (task_one->util > task_two->util) return -1;
    return 0;
}


int main(int argc, char *argv[]) {
    //Scan taskset.txt -> store in input
    if(argc != 1) {
        printf("Extra arguments\n");
        return 1;
    }
    FILE *file = fopen("taskset.txt", "r");
    if(!file) {
        printf("Error opening file\n");
        return 1;
    }

    int rows_read = 0;
    Task tasks[NUM_TASKS];

    int C, T, D, chain_id, chain_pos;
    while(fscanf(file, "%d %d %d %d %d", &C, &T, &D, &chain_id, &chain_pos) == 5) {
        tasks[rows_read].C_ms = C;
        tasks[rows_read].T_ms = T;
        tasks[rows_read].D_ms = D;
        tasks[rows_read].chain_id = chain_id;
        tasks[rows_read].chain_pos = chain_pos;

        tasks[rows_read].util = (double)C / (double)T; //Set util to C / T
        tasks[rows_read].id = rows_read;
        rows_read++;
    }
    fclose(file);

    qsort(tasks, rows_read, sizeof(Task), compare_task_util); //sort tasks by util (to be used later for first fit decreasing)

    double cpu_util[NUM_CORES] = {0.0, 0.0};

    for(int i = 0; i < rows_read; i++) { //select assigned_core using first fit decreasing algorithm
        if(cpu_util[0] + tasks[i].util <= 1.0) { //If it fits in cpu 0, set assigned_core to 0
            cpu_util[0] += tasks[i].util;
            tasks[i].assigned_core = 0;
        }
        else if(cpu_util[1] + tasks[i].util <= 1.0) { //Else if it fits in cpu 1, set assigned_core to 1
            cpu_util[1] += tasks[i].util;
            tasks[i].assigned_core = 1;
        }
        else { //Else if it doesn't fit in either throw error
            printf("Error: Task %d could not fit on either core\n", tasks[i].id);
            printf("Current cpu utils: %.2f, %.2f\n", cpu_util[0], cpu_util[1]);
            return 1;
        }
    }

    for(int i = 0; i < rows_read; i++) { //Print which core each task is assigned to in order of id
        for(int j = 0; j < rows_read; j++) {
            if(tasks[j].id == i) {
                printf("Task %d assigned to core %d\n", tasks[j].id, tasks[j].assigned_core);
                continue;
            }
        }
    }

    calibrate();
    pid_t pid_list[NUM_TASKS];

    for(int i = 0; i < rows_read; i++) {
        pid_t pid = fork();
        if(pid < 0) {
            printf("Fork failed\n");
            return 1;
        } else if(pid == 0) { //child process
            //Set the task to the desired core | Documentation: https://man7.org/linux/man-pages/man2/sched_setaffinity.2.html
            cpu_set_t mask;
            CPU_ZERO(&mask);
            CPU_SET(tasks[i].assigned_core, &mask);
            if(sched_setaffinity(0, sizeof(mask), &mask) == -1) { //0 means the current process
                printf("sched_setaffinity error\n");
                return 1;
            }

            //call set_edf_task
            struct timespec C = ms_to_timespec(tasks[i].C_ms);
            struct timespec T = ms_to_timespec(tasks[i].T_ms);
            struct timespec D = ms_to_timespec(tasks[i].D_ms);

            if(set_edf_task(getpid(), &C, &T, &D, 
                            tasks[i].assigned_core,
                            tasks[i].chain_id,
                            tasks[i].chain_pos) < 0) {
                printf("set_edf_task failed\n");
                return 1;
            }

            //Work loop
            while(1) {
                dummy_load(tasks[i].C_ms);
                long wret = wait_until_next_period();
                if(wret < 0) {
                    printf("wait_until_next_period failed\n");
                    exit(1);
                }
            }
            exit(0);
        }
        else { //parent process
            pid_list[i] = pid;
        }
    }

    sleep(120); //wait 2 minutes

    for(int i = 0; i < rows_read; i++) { //cancel all tasks
        cancel_rsv(pid_list[i]);
        kill(pid_list[i], SIGKILL);
    }

    return 0;
}
