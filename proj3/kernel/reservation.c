#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/time.h>
#include <linux/errno.h>
#include <linux/printk.h>


long (*set_rsv_hook)(pid_t, const struct timespec __user *, const struct timespec __user *);
EXPORT_SYMBOL(set_rsv_hook);

struct task_struct *rsv_get_task_by_pid(pid_t pid) {
	struct task_struct *task = NULL;

	pr_info("***i am inside the get task\n\n");
	//find target_task's task_struct using provided pid
	if(pid == 0){
		task = current;
		pr_info("PID = 0\n\n");
		return task;
	}else{
		rcu_read_lock();
		pr_info("***i am inside teh rcu read lock\n\n");
		task = find_task_by_pid_ns(pid, task_active_pid_ns(current));
		if(task)
			get_task_struct(task);
		pr_info("***i am after the find task\n\n");
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
	pr_emerg("set_rsv: ENTER pid%d\n", pid);
	pr_info("seet_rsv syscall enetered (pid=%d)\n", pid);
	
	if(set_rsv_hook)
		return set_rsv_hook(pid, C, T);
		
	pr_info("set_rsv: no hook loaded\n");
	return -ENOSYS;
}

SYSCALL_DEFINE1(cancel_rsv, pid_t, pid){
	return;
}

SYSCALL_DEFINE0(wait_until_next_period){
	return;
}

