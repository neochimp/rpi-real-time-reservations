#include <linux/module.h> //Core module stuff
#include <linux/kernel.h> //Core kernel stuff (things like macros)
#include <linux/syscalls.h> //For system calls
#include <linux/uaccess.h> //copy_to_user, copy_from_user
#include <linux/errno.h> //For error numbers
#include <linux/types.h> //Not sure if I'll use this
#include <linux/export.h> //For exported symbols

//Module metadata
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group11");
MODULE_DESCRIPTION("mod_calc: overrides other calculator functionality with modulo");

//Defining function-pointer type
typedef asmlinkage long (*calc_t)(int, int, char, int __user*);

//Taking original implementation from kernel export
extern asmlinkage calc_t calc_curr;

//So we can save the old version for restoring after removing the module
static calc_t calc_old;

//new version (modulo version)
static asmlinkage long calc_modulo(int param1, int param2, char operation, int __user *result) {
	int output;
	if(param2 == 0) { //can't do param1 % 0
		return -1;
	}
	output = param1 % param2;
	
	//Attempt to copy to result to verify that the address works properly	
	if(copy_to_user(result, &output, sizeof(output))) {
		return -EFAULT;
	}
	return 0; //worked if it makes it here
}

//For starting the module
static int __init mod_calc_init(void) {
	calc_old = calc_curr;
	calc_curr = calc_modulo;
	return 0; //tells the kernel that initialization was successful
}

//For removing the module
static void __exit mod_calc_exit(void) {
	calc_curr = calc_old;
}

module_init(mod_calc_init); //call mod_calc_init on insmod
module_exit(mod_calc_exit); //call mod_calc_exit on rmmod
