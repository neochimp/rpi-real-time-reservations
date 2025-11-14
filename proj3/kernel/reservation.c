#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/hrtimer.h>
#include <linux/pid.h>
#include <linux/ktime.h>
#include <linux/spinlock.h>

#define NSEC_PER_SEC 1000000000LL
#define MSEC_PER_SEC 1000.0


task_struct* rsv_get_task_by_pid(pid_t pid) {
	struct task_struct *target_task = NULL;

    // find target_task's task_struct using provided pid
    if (pid == 0)
        target_task = current;
    else
    {
        rcu_read_lock();
        target_task = find_task_by_pid_ns(pid, task_active_pid_ns(current));
        rcu_read_unlock();
    }

    return target_task;
}

/*
 * Assigns or "reserves" a CPU time budget (C) over a period (T) for a specific task
 */
SYSCALL_DEFINE3(set_rsv, 
                pid_t, pid, 
                struct timespec __user *, C, 
                struct timespec __user *, T)
{
    /*
     * task_struct *target_task
     * find target_task's task_struct from pid
     * if:
     *     C and T are valid user-space addresses (copy_from_user())
     *     target task doesn't already have an active reservation
     *     continue
     *  else:
     *      return -1
     * store C, T into target_tasks's task_struct
     * initialize kernel mechanisms (timers) to manage the reservation
     * return 0
     */

    // struct task_struct current; defined in <linux/sched.h>
	struct task_struct *target_task;
    struct timespec C_local, T_local;

    // find target_task's task_struct using provided pid
    target_task = rsv_get_task_by_pid(pid);
    if (!target_task)
        return -1;

    // C and T are valid addresses from user space
    if (copy_from_user(&C_local, C, sizeof(*C)) ||
        copy_from_user(&T_local, T, sizeof(*T)))
        return -1;
    
    long long C_ns = C_local.tv_sec * NSEC_PER_SEC + C_local.tv_nsec;
    long long T_ns = T_local.tv_sec * NSEC_PER_SEC + T_local.tv_nsec;

    // C and T are valid times
    if (C_ns < 0 || T_ns < C_ns)
        return -1;

    // target already has an active reservation
    if (target_task->rsv_active)
        return -1;
    
    // store C, T into target_task's task_struct
    target_task->rsv_C = C_local;
    target_task->rsv_T = T_local;
    target_task->rsv_active = true;

    hrtimer_init(target_task->rsv_timer_pointer, CLOCK_MONOTONIC, HRTIMER_MODE_REL_PINNED);
    target_task->rsv_timer_pointer
    
    return 0;
}


/*
 * Removes an existing CPU time reservation from a task
 */
SYSCALL_DEFINE1(cancel_rsv, 
                pid_t, pid)
{
	/*
     * task_struct *target_task
     * find target_task's task_struct from pid
     * if reservation exists:
     *     tear down timers
     *     clear the stored C and T values
     *     free any allocated resources
     *     return 0
     * else:
     *     return -1
     */
    struct task_struct *target_task;

    target_task = rsv_get_task_by_pid(pid);
    if (!target_task)
        return -1;
    if (!target_task->rsv_active)
        return -1;

    target_task->rsv_C = NULL;
    target_task->rsv_T = NULL;
    target_task->rsv_active = false;

}

/*
 * Suspends the calling task until the begining of its next reservation period
 */
SYSCALL_DEFINE0(wait_until_next_period) 
{
	/*
    * check if current task has an active reservation
    * if no active reservation:
    *     return -1
    * else:
    *     put current to sleep (set state to TASK_INTERRUPTIBLE)
    *     wait for periodic timer mechanism 
    *     upon waking, return 0
    */
}
