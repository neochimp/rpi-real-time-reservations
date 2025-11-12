//Just a skeleton for now
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/hrtimer.h>

/*
 * Assigns or "reserves" a CPU time budget (C) over a period (T) for a specific task
 */
SYSCALL_DEFINE3(set_rsv, 
                pid_t, pid, 
                struct timespec __user *, C, 
                struct timespec __user *, T)
{
	return 0;
{

/*
 * Removes an existing CPU time reservation from a task
 */
SYSCALL_DEFINE1(cancel_rsv, 
                pid_t, pid)
{
	return 0;
}

/*
 * Suspends the calling task until the begining of its next reservation period
 */
SYSCALL_DEFINE0(wait_until_next_period) 
{
	return 0;
}
