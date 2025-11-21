#include <linux/errno.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/time.h>

extern long (*set_rsv_hook)(pid_t, const struct timespec __user *,
                            const struct timespec __user *);

static long my_set_rsv(pid_t pid, const struct timespec __user *C,
                       const struct timespec __user *T) {
  pr_info("I GOT THIS FROM THE SSH (pid=%d)\n", pid);
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
