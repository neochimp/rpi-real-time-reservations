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
#include <linux/syscalls.h>
#include <linux/cpumask.h> //nr_cpu_ids

//4.3 RT priority assignment
#define MAX_RSV 50
 
struct rsv_entry {
        struct task_struct *task;
        u64 T_ns; //I don't think we need this anymore but I'm going to leave it anyway in case I miss a reference to it somewhere
        bool used;
};

static struct rsv_entry rsv_table[MAX_RSV]; //table to hold all reservations
static DEFINE_SPINLOCK(rsv_lock); //Reference documentation: https://www.kernel.org/doc/Documentation/locking/spinlocks.txt

static enum hrtimer_restart rsv_timer_callback(struct hrtimer *timer); //forward declaration

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
};

struct edf_task_struct {
	const struct timespec __user * C;
	const struct timespec __user * T;
	const struct timespec __user * D;
	int cpu_id;
	int chain_id;
	int chain_pos;
};

//helper function for printing out the current list of tasks.
static void printTasks(void){
        int i;
        pr_info("**Current tasks scheduled:\n");
        for(i = 0; i < MAX_RSV; i++){
                if(rsv_table[i].used && rsv_table[i].task){
                        struct task_struct *p = rsv_table[i].task;
                        pr_info("   PID: %d, D: %lld, Priority: %d\n", p->pid, (long long)(rsv_table[i].task->rsv_D.tv_sec*NSEC_PER_SEC+rsv_table[i].task->rsv_D.tv_nsec), p->rt_priority);
                }
        }
}

//rsv table functions
static void rsv_update_priorities(void) {
    struct task_struct *edf_tasks[MAX_RSV];
    int i, j, task_num;
    int cpu;

    //Each cpu needs to be treated independently
    for_each_online_cpu(cpu) { //I couldn't find much but this is the best I've got - Documentation: https://lwn.net/Articles/537570/
        task_num = 0; //The current task

        //Update edf_tasks so it accurately reflects all active tasks
        for(i = 0; i < MAX_RSV; i++) {
            struct task_struct *temp;

            if(!rsv_table[i].used) { //Verify that the task is being used
                continue;
            }

            temp = rsv_table[i].task; //Store the current task
            if(!temp || !temp->rsv_active) { //Verify that the current task exists and is active
                continue;
            }
            if(temp->rsv_cpu_id != cpu) { //Verify that rsv_cpu_id matches cpu
                continue;
            }

            edf_tasks[task_num++] = temp; //set edf_tasks[taskNum] to temp and then increment taskNum
        }
        
        //Sort edf_tasks by rsv_abs_deadline_ns (bubble sort)
        for(i = 0; i < task_num - 1; i++) {
            for(j = 0; j < task_num - 1 - i; j++) {
                if(edf_tasks[j]->rsv_abs_deadline_ns > edf_tasks[j + 1]->rsv_abs_deadline_ns) {
                    struct task_struct *temp = edf_tasks[j];
                    edf_tasks[j] = edf_tasks[j + 1];
                    edf_tasks[j + 1] = temp;
                }
            }
        }
        
        //Assign priorities
        for(i = 0; i < task_num; i++) {
            int prio = (MAX_RT_PRIO - 1) - i; //The earliest tasks in edf_tasks / the tasks with the earliest deadlines get the highest priorities (start from 99 and count down)
            struct sched_param param = {0};
            param.sched_priority = prio;
            int ret;

            ret = sched_setscheduler_nocheck(edf_tasks[i], SCHED_FIFO, &param);

            if(ret) { //if return error
                pr_info("sched scheduler failed for pid%d, ret%d\n", edf_tasks[i]->pid, ret);
            }
            else { //success
                //pr_info("set pid=%d SCHED_FIFO rt_prio=%d", edf_tasks[i]->pid, prio);
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
    			printTasks();
			return;
                }
        }
}


static void rsv_table_remove(struct task_struct *curr_task) {
        int i;
        for(i = 0; i < MAX_RSV; i++){
                if(rsv_table[i].used && rsv_table[i].task == curr_task){
                        rsv_table[i].used = false;
                        rsv_table[i].task = NULL;
                        rsv_table[i].T_ns = 0;

                        rsv_update_priorities();
    			printTasks();
			return;
                }
        }
        //TODO: condition for if not found
}


//helper function for clarity
static void resetAccumulator(struct task_struct *task){
        task->rsv_accumulated_ns = 0;

}


SYSCALL_DEFINE2(set_edf_task,
                pid_t, pid, struct edf_task_struct __user*, edf_info)
{
	//breaking down the edf_task_struct.
        struct task_struct *target_task;       
	struct timespec* C = edf_info->C;
	struct timespec* T = edf_info->T;
	struct timespec* D = edf_info->D;
	int cpu_id = edf_info->cpu_id;
	int chain_id = edf_info->chain_id;
	int chain_pos = edf_info->chain_pos;

        // current task struct, defined in linux/sched.h
        //find the target task_struct using the pid
        target_task = rsv_get_task_by_pid(pid);
        if(!target_task){
                pr_info("target task fail\n");
                return -1;
        }

        //check if C & T & D are valid user space addresses
        if(!C || !T || !D){
                pr_info("PID:%d C or T fail\n", pid);
                return -1;
        }

        u64 C_ns = (u64)C->tv_sec * NSEC_PER_SEC + (u64)C->tv_nsec;
        u64 T_ns = (u64)T->tv_sec * NSEC_PER_SEC + (u64)T->tv_nsec;

        /* PROJECT 4 CHANGES START HERE */
        u64 now = ktime_get_ns();
        u64 D_ns = (u64)D->tv_sec * NSEC_PER_SEC + (u64)D->tv_nsec;

        if(cpu_id < 0 || cpu_id >= nr_cpu_ids) {
            pr_info("set_edf_task: invalid cpu_id %d\n", cpu_id);
            return -1;
        }
        
        target_task->rsv_cpu_id = cpu_id;
        /* PROJECT 4 CHANGES END HERE */
        
        //C, T, and D are valid times
        if(C_ns <= 0 || D_ns <= 0 || D_ns < C_ns){
                pr_info("PID:%d C or T value fail\n", pid);
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
    		unsigned long flags;
        	spin_lock_irqsave(&rsv_lock, flags);
		rsv_update_priorities();
        	spin_unlock_irqrestore(&rsv_lock, flags);
        }else{
                hrtimer_cancel(&target_task->rsv_timer);
        }

        //store C, T into target_task's task_struct
        target_task->rsv_C = *C;
        target_task->rsv_T = *T;
        target_task->rsv_active = true;
        target_task->rsv_accumulated_ns = 0;
        target_task->rsv_last_start_ns = 0;

        /* PROJECT 4 CHANGES START HERE */
        target_task->rsv_D = *D;
        target_task->rsv_abs_deadline_ns = now + D_ns;

        target_task->rsv_chain_id = chain_id;
        target_task->rsv_chain_pos = chain_pos;

        //create a cpu mask and use it to pin the target task to the cpu_id
        //There's horrible documentation for this so I have zero confidence in the accuracy of the next four lines
        //Can't tell if this needs to be done here for 4.1 or in user space in 4.2 so I'm commenting it out for now
        //struct cpumask mask;
        //cpumask_clear(&mask);
        //cpumask_set_cpu(cpu_id, &mask);
        //set_cpus_allowed_ptr(target_task, &mask);
        /* PROJECT 4 CHANGES END HERE */
        
        pr_info("PID:%d: Successfully set task values C: %lld, T: %lld, D: %lld\n", pid, (long long)(target_task->rsv_C.tv_sec*NSEC_PER_SEC+target_task->rsv_C.tv_nsec), (long long)(target_task->rsv_T.tv_sec*NSEC_PER_SEC+target_task->rsv_T.tv_nsec), (long long)(target_task->rsv_D.tv_sec*NSEC_PER_SEC+target_task->rsv_D.tv_nsec));


        hrtimer_start(&target_task->rsv_timer, ns_to_ktime(T_ns), HRTIMER_MODE_REL_PINNED); //start the timer

        //lock critical section
        unsigned long flags;
        spin_lock_irqsave(&rsv_lock, flags);
        rsv_table_add(target_task, T_ns);
        spin_unlock_irqrestore(&rsv_lock, flags);
        
        return 0; // success
}

SYSCALL_DEFINE1(cancel_rsv, pid_t, pid){
        struct task_struct *target_task;
        target_task = rsv_get_task_by_pid(pid);

        if(!target_task)
                return -1;
                
        if(!target_task->rsv_active){
                pr_info("cancel_rsv: No reservation on pid%d\n", pid);
        	return -1;
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

        /* PROJECT 4 CHANGES START HERE */
        target_task->rsv_D.tv_sec = 0;
        target_task->rsv_D.tv_nsec = 0;

        target_task->rsv_cpu_id = 0;
        target_task->rsv_chain_id = 0;
        target_task->rsv_chain_pos = 0;
        target_task->rsv_abs_deadline_ns = 0;
        /* PROJECT 4 CHANGES END HERE */

        unsigned long flags;
        spin_lock_irqsave(&rsv_lock, flags);
        rsv_table_remove(target_task);
        spin_unlock_irqrestore(&rsv_lock, flags);

        return 0;
}

SYSCALL_DEFINE0(wait_until_next_period){
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

SYSCALL_DEFINE2(get_e2e_latency,
                int, chain_id,
                struct timespec __user *, latency_buf)
{
    //TODO: Implement this
    return -1;
}



//hrtimer callback function
static enum hrtimer_restart rsv_timer_callback(struct hrtimer *timer) {
    //get the task_struct by using the hrtimer
    struct task_struct *task = container_of(timer, struct task_struct, rsv_timer);
    u64 now;
    
    if(!task) {
        pr_info("rsv_timer_callback: task is NULL\n");
        return HRTIMER_NORESTART;
    }

    if(!task->rsv_active)
        return HRTIMER_NORESTART;
   
    now = ktime_get_ns();
   	
    
    if(task->rsv_last_start_ns) {
   	u64 delta = now - task->rsv_last_start_ns;
   	task->rsv_accumulated_ns += delta;
   	task->rsv_last_start_ns = now;
    }

    //4.5 accumulator
    u64 C_ns = timespec_to_ns(&task->rsv_C);
    u64 used = task->rsv_accumulated_ns;
    
    if(C_ns > 0 && used > C_ns) {
        u64 util_pct = div64_u64(used * 1000, C_ns);
        u64 mod = do_div(util_pct, 10);
        printk(KERN_INFO "Task %d: budget overrun (util: %llu.%llu%%)\n", task->pid, util_pct, mod);
        
        //4.6 overrun notification
        send_sig(SIGUSR1, task, 0);
    }

    //reset accumulator for next task
    resetAccumulator(task);

    /* PROJECT 4 CHANGES START HERE */
    //Update rsv_abs_deadline_ns and all priorities (because changing a task's absolute deadline also changes its priority)
    u64 D_ns = (u64)task->rsv_D.tv_sec * NSEC_PER_SEC + (u64)task->rsv_D.tv_nsec;
    task->rsv_abs_deadline_ns = now + D_ns;

        /* PROJECT 4 CHANGES START HERE */

    task->rsv_period_elapsed = true;
    wake_up_interruptible(&task->rsv_wq);

    //rearm
    hrtimer_forward_now(timer, timespec_to_ktime(task->rsv_T));
    return HRTIMER_RESTART;
}
