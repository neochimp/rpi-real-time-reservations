#define _GNU_SOURCE
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/pid.h>
#include <linux/ktime.h>
#include <linux/spinlock.h>
#include <linux/sched/prio.h> //MAX_RT_PRIO
#include <linux/wait.h>
#include <linux/hrtimer.h>
#include <linux/math64.h>
#include <linux/signal.h>

//4.3 RT priority assignment
#define MAX_RSV 50

struct rsv_entry {
	struct task_struct *task;
	u64 T_ns;
	bool used;
};

static struct rsv_entry rsv_table[MAX_RSV]; //table to hold all reservations
static DEFINE_SPINLOCK(rsv_lock); //Reference documentation: https://www.kernel.org/doc/Documentation/locking/spinlocks.txt

static enum hrtimer_restart rsv_timer_callback(struct hrtimer *timer); //forward declaration

//rsv table functions
static void rsv_update_priorities(void){
	u64 unique_periods[MAX_RSV];
	int num_unique = 0;
	int i, j;

	//find all unique periods
	for(i = 0; i < MAX_RSV; i++){
		if(rsv_table[i].used){
			u64 T = rsv_table[i].T_ns;
			bool found = false;
			//look through unique periods to see if value is already there
			for(j = 0; j < num_unique; j++){
				if(unique_periods[j] == T){
					found = true;
					break;
				}
			}
			//if not found in unique_periods, add it and increment num_unique
			if(!found){
				unique_periods[num_unique++] = T;
			}
		}

	}
	//sort unique periods with insertion sort
	for(i = 1; i < num_unique; i++){
		u64 key = unique_periods[i];
		j = i - 1;
		while(j >= 0 && unique_periods[j] > key){
			unique_periods[j+1] = unique_periods[j];
			j--;
		}
		unique_periods[j+1] = key;
	}

	//doing the priority updating
	for(i = 0; i < num_unique; i++){
		int prio = (MAX_RT_PRIO - 1) -i;
		if(prio < 0)
			prio = 0;

		for(j = 0; j < MAX_RSV; j++){
			if(rsv_table[j].used && 
				rsv_table[j].T_ns == unique_periods[i] &&
				rsv_table[j].task){

				struct sched_param param = {0};
				int ret;
				param.sched_priority = prio;
				ret = sched_setscheduler_nocheck(rsv_table[j].task, SCHED_FIFO, &param);
				if(ret){
					pr_info("sched scheduler failed for pid%d, ret%d\n", rsv_table[j].task->pid, ret);
				}else{
					pr_info("set pid=%d SCHED_FIFO rt_prio=%d", rsv_table[j].task->pid, prio);
				}
			}
		}
	}
}


static void rsv_table_add(struct task_struct *curr_task, u64 T_ns){
	int i;
	for(i = 0; i < MAX_RSV; i++){ //find the first unused spot and insert there
		if(!rsv_table[i].used) {
			rsv_table[i].used = true;
			rsv_table[i].task = curr_task;
			rsv_table[i].T_ns = T_ns;
			
			rsv_update_priorities();
			return;
		}
	}
	//TODO: condition for if table is full
}

static void rsv_table_remove(struct task_struct *curr_task) {
	int i;
	for(i = 0; i < MAX_RSV; i++){
		if(rsv_table[i].used && rsv_table[i].task == curr_task){
			rsv_table[i].used = false;
			rsv_table[i].task = NULL;
			rsv_table[i].T_ns = 0;
			return;
		}
	}
	//TODO: condition for if not found
}

//helper function for printing out the current list of tasks.
static void printTasks(void){
	int i;
	pr_info("**Current tasks scheduled:\n");
	for(i = 0; i < MAX_RSV; i++){
		if(rsv_table[i].used && rsv_table[i].task){
			struct task_struct *p = rsv_table[i].task;
			pr_info("   PID: %d, T: %lld, Priority: %d\n", p->pid, rsv_table[i].T_ns, p->rt_priority);
		}
	}
}
//helper function for clarity
static void resetAccumulator(struct task_struct *task){
	task->rsv_accumulated_ns = 0;
	pr_info("accumulator reset\n");

}
extern long (*set_rsv_hook)(pid_t, const struct timespec __user *,
                            const struct timespec __user *);
extern long (*cancel_rsv_hook)(pid_t);
extern long (*wait_until_next_period_hook)(void);
extern struct task_struct *rsv_get_task_by_pid(pid_t pid);

static long mod_set_rsv(pid_t pid, const struct timespec __user *C,
                       const struct timespec __user *T) {
  	pr_info("entered mod\n");
	// current task struct, defined in linux/sched.h
	struct task_struct *target_task;	
  	//find the target task_struct using the pid
	target_task = rsv_get_task_by_pid(pid);
	if(!target_task){
		pr_info("target task fail\n");
		return -1;
	}

	//check if C & T are valid user space addresses
	if(!C || !T){
		pr_info("C and T fail\n");
		return -1;
	}

	u64 C_ns = (u64)C->tv_sec * NSEC_PER_SEC + (u64)C->tv_nsec;
	u64 T_ns = (u64)T->tv_sec * NSEC_PER_SEC + (u64)T->tv_nsec;
	
	//C and T are valid times
	if(C_ns <= 0 || T_ns < C_ns){
		pr_info("C and T value fail\n");
		return -1;
	}

	//target already has an active reservation
	if(target_task->rsv_active){
		pr_info("target already has active reservation\n");
		return -1;
	}

	//part 4.4 conditions used to wake the timer for wait_until_next_period();
	if(!target_task->rsv_active){
		init_waitqueue_head(&target_task->rsv_wq); //declare rsv_wq
		target_task->rsv_period_elapsed = false;
		hrtimer_init(&target_task->rsv_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL_PINNED); //define the timer
		target_task->rsv_timer.function = rsv_timer_callback; //set the callback function to our custom function
	}else{
		hrtimer_cancel(&target_task->rsv_timer);
	}

	//store C, T into target_task's task_struct
	target_task->rsv_C = *C;
	target_task->rsv_T = *T;
	target_task->rsv_active = true;
	
	pr_info("Successfully set task values C: %lld, T: %lld\n", (long long)(target_task->rsv_C.tv_sec*NSEC_PER_SEC+target_task->rsv_C.tv_nsec), (long long)(target_task->rsv_T.tv_sec*NSEC_PER_SEC+target_task->rsv_T.tv_nsec));


	hrtimer_start(&target_task->rsv_timer, ns_to_ktime(T_ns), HRTIMER_MODE_REL_PINNED); //start the timer

	//lock critical section
	unsigned long flags;
	spin_lock_irqsave(&rsv_lock, flags);
	rsv_table_add(target_task, T_ns);
	spin_unlock_irqrestore(&rsv_lock, flags);
	
	
	.return 0; // success
}

static long mod_cancel_rsv(pid_t pid){
	struct task_struct *target_task;
	target_task = rsv_get_task_by_pid(pid);

	if(!target_task)
		return -1;
	if(!target_task->rsv_active){
		pr_info("cancel_rsv: No reservation on pid%d\n", pid);
	}

	target_task->rsv_C.tv_sec = 0;
	target_task->rsv_C.tv_nsec = 0;

	target_task->rsv_T.tv_sec = 0;
	target_task->rsv_T.tv_nsec = 0;
	target_task->rsv_active = false;

	//part 4.4 
	hrtimer_cancel(&target_task->rsv_timer); //cancel timer
	wake_up_interruptible(&target_task->rsv_wq); //wake up wait_until_next_period()
	target_task->rsv_period_elapsed = false; //clear the condition used by wait_until_next_period()

	pr_info("cancel_rsv: Reservation for pid%d cancelled", pid);
	
	
	//part 4.5
	target_task->rsv_accumulated_ns = 0;
	target_task->rsv_last_start_ns = 0;

	unsigned long flags;
	spin_lock_irqsave(&rsv_lock, flags);
	rsv_table_remove(target_task);
	spin_unlock_irqrestore(&rsv_lock, flags);

	return 0;
}

static long mod_wait_until_next_period(void){
	pr_info("waiting until next period\n");	
	//check if there is an active rsv
	if(!current->rsv_active){
		pr_info("mod_wait_until_next_period: no active reservation");
		return -1;
	}

	current->rsv_period_elapsed = false;

	//if wait_event_interruptible returns true, something wrong happened.
	if(wait_event_interruptible(current->rsv_wq, current->rsv_period_elapsed)) {
		return -1;
	}
	//check if rsv cancelled while slept
	if(!current->rsv_active){
		pr_info("mod_wait_until_next_period: reservation cancelled while slept");
		return -1;
	}

	return 0;
}

//hrtimer callback function
static enum hrtimer_restart rsv_timer_callback(struct hrtimer *timer) {
	//get the task_struct by using the hrtimer
	struct task_struct *task = container_of(timer, struct task_struct, rsv_timer);
	
	
	if(!task){
		pr_info("rsv_timer_callback: task is NULL\n");
		return HRTIMER_NORESTART;
	}

	if(!task->rsv_active)
		return HRTIMER_NORESTART;

	//4.5 accumulator
	u64 C_ns = timespec_to_ns(&task->rsv_C);
	u64 used = task->rsv_accumulated_ns;
	
	printk(KERN_INFO "Pre-check C: %llu, used: %llu\n", C_ns, used);
	if(C_ns > 0 && used > C_ns){
		u64 util_pct = div64_u64(used * 1000, C_ns);
		u64 mod = do_div(util_pct, 10);
		printk(KERN_INFO "Task %d: budget overrun (util: %llu.%llu%%)\n", task->pid, util_pct, mod);
		
		//4.6 overrun notification
		//send_sig(SIGUSR1, task, 0);
	}

	//reset accumulator for next task
	resetAccumulator(task);


	task->rsv_period_elapsed = true;
	wake_up_interruptible(&task->rsv_wq);

	//rearm
	hrtimer_forward_now(timer, timespec_to_ktime(task->rsv_T));
	return HRTIMER_RESTART;
}



static int __init mod_init(void) {
  set_rsv_hook = mod_set_rsv;
  cancel_rsv_hook = mod_cancel_rsv;
  wait_until_next_period_hook = mod_wait_until_next_period;
  pr_info("hooks installed\n");
  return 0;
}

static void __exit mod_exit(void) {
  set_rsv_hook = NULL;
  pr_info("set_rsv hook removed\n");
}

module_init(mod_init);
module_exit(mod_exit);
MODULE_LICENSE("GPL");
