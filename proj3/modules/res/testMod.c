#include <linux/errno.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/pid.h>
#include <linux/ktime.h>
#include <linux/spinlock.h>

extern long (*set_rsv_hook)(pid_t, const struct timespec __user *,
                            const struct timespec __user *);

extern struct task_struct *rsv_get_task_by_pid(pid_t pid);

static long my_set_rsv(pid_t pid, const struct timespec __user *C,
                       const struct timespec __user *T) {
  	// current task struct, defined in linux/sched.h
	struct task_struct *target_task;	
  	//find the target task_struct using the pid
	target_task = rsv_get_task_by_pid(pid);
	if(!target_task)
		return -1;

	//check if C & T are valid user space addresses
	if(!C || !T)
		return -1;

	u64 C_ns = (u64)C->tv_sec * NSEC_PER_SEC + (u64)C->tv_nsec;
	u64 T_ns = (u64)T->tv_sec * NSEC_PER_SEC + (u64)T->tv_nsec;
	
	//C and T are valid times
	if(C_ns <= 0 || T_ns < C_ns)
		return -1;

	//target already has an active reservation
	if(target_task->rsv_active)
		return -1;

	//store C, T into target_task's task_struct
	target_task->rsv_C = *C;
	target_task->rsv_T = *T;
	target_task->rsv_active = true;
	
	pr_info("Successfully set task values C: %ld, T: %ld\n", target_task->rsv_C.tv_sec*NSEC_PER_SEC+C->tv_nsec, target_task->rsv_T.tv_sec*NSEC_PER_SEC+T->tv_nsec);

	return 0; // success
}

static int __init mod_init(void) {
  set_rsv_hook = my_set_rsv;
  pr_info("set_rsv hook installed\n");
  return 0;
}

static void __exit mod_exit(void) {
  set_rsv_hook = NULL;
  pr_info("set_rsv hook removed\n");
}

module_init(mod_init);
module_exit(mod_exit);
MODULE_LICENSE("GPL");
