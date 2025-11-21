#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/time.h>
#include <linux/errno.h>
#include <linux/printk.h>


long (*set_rsv_hook)(pid_t, const struct timespec __user *, const struct timespec __user *);
EXPORT_SYMBOL(set_rsv_hook);

SYSCALL_DEFINE3(set_rsv,
                pid_t, pid,
                const struct timespec __user *, C,
                const struct timespec __user *, T)
{
	pr_emerg("set_rsv: ENTER pid%d\n", pid);
	pr_info("seet_rsv syscall enetered (pid=%d)\n", pid);
	
	if(set_rsv_hook)
		return set_rsv_hook(pid, C, T);
		
	pr_info("set_rsv: no hook loaded\n");
	return -ENOSYS;
}

SYSCALL_DEFINE1(cancel_rsv, pid_t, pid){

}

SYSCALL_DEFINE0(wait_until_next_period){

}

