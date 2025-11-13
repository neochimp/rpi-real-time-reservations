//Just a skeleton for now
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/hrtimer.h>
#include <linux/pid.h>
#include <linux/ktime.h>
#include <linux/spinlock.h>

/*
 * Assigns or "reserves" a CPU time budget (C) over a period (T) for a specific task
 */
SYSCALL_DEFINE3(set_rsv, 
                pid_t, pid, 
                struct timespec __user *, C, 
                struct timespec __user *, T)
{
    // struct task_struct current; defined in <linux/sched.h>
	struct task_struct *target_task;
    struct pid *kernel_pid;
    struct timespec C_local, T_local;

    // checking task's task_struct using provided pid
    if (pid == 0)
        target_task = current;
    else {
        kernel_pid = find_vpid(pid);
        if (!kernel_pid)
            return -1;

        target_task = pid_task(kernel_pid, PIDTYPE_PID);
        if (!target_task)
            return -1;
    }


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
