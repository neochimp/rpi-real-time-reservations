#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/time.h>
#include <linux/errno.h>
#include <linux/printk.h>


long (*set_rsv_hook)(pid_t, const struct timespec __user *, const struct timespec __user *);
EXPORT_SYMBOL(set_rsv_hook);

long (*cancel_rsv_hook)(pid_t);
EXPORT_SYMBOL(cancel_rsv_hook);

long (*wait_until_next_period_hook)(void);
EXPORT_SYMBOL(wait_until_next_period_hook);


struct task_struct *rsv_get_task_by_pid(pid_t pid) {
	struct task_struct *task = NULL;

	//find target_task's task_struct using provided pid
	if(pid == 0){
		task = current;
		pr_info("PID = 0\n\n");
		return task;
	}else{
		rcu_read_lock();
		task = find_task_by_pid_ns(pid, task_active_pid_ns(current));
		if(task)
			get_task_struct(task);
		rcu_read_unlock();
	}
	return task;
}
EXPORT_SYMBOL(rsv_get_task_by_pid);

SYSCALL_DEFINE3(set_rsv,
                pid_t, pid,
                const struct timespec __user *, C,
                const struct timespec __user *, T)
{
	pr_info("set_rsv: sys_call entered (pid=%d)\n", pid);
	
	if(set_rsv_hook)
		return set_rsv_hook(pid, C, T);
		
	pr_info("set_rsv: no hook loaded\n");
	return -ENOSYS;
}

SYSCALL_DEFINE1(cancel_rsv, pid_t, pid){

	pr_info("cancel_rsv: sys_call entered (pid=%d)\n", pid);
	
	if(cancel_rsv_hook)
		return cancel_rsv_hook(pid);
		
	pr_info("set_rsv: no hook loaded\n");
	return -ENOSYS;
}

SYSCALL_DEFINE0(wait_until_next_period){
	pr_info("wait_until_next_period: sys_call entered");

	if(wait_until_next_period_hook)
		return wait_until_next_period_hook();

	pr_info("wait_until_next_period: no hook loaded\n");
	return -ENOSYS;
}

