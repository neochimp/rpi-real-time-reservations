#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Cameron Lee");
MODULE_DESCRIPTION("Hello world LKM using printk for Project 2 part 5.2");

static int __init lkmhello_init(void){
  printk(KERN_INFO "Hello world! Group11 in kernel space\n");
  return 0;
}

module_init(lkmhello_init);


