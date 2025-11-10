//Just a skeleton for now
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/hrtimer.h>

SYSCALL_DEFINE3(set_rsv, pid_t, pid, struct timespec __user *, C, struct timespec __user *, T) {
	return 0;
{

SYSCALL_DEFINE1(cancel_rsv, pid_t, pid) {
	return 0;
}

SYSCALL_DEFINE1(wait_until_next_period) {
	return 0;
}
