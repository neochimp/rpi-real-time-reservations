#include <linux/syscalls.h> //To modify system calls
#include <linux/kernel.h> //Not sure how necessary this is but every example seems to have it
#include <linux/uaccess.h> //for pointer references
#include <linux/errno.h> //-EFAULT
#include <linux/export.h> //EXPORT_SYMBOL

//defining a function-pointer type so that we can later create variations of it depending on if a modules is implemented or not to be called by the calc system call
typedef asmlinkage long (*calc_t)(int, int, char, int __user * );

//default implementation
static asmlinkage long calc_default(int param1, int param2, char operation, int __user *result) {
	int output;
	switch(operation) {
		case '+':
			output = param1 + param2;
			break;
		case '-':
			output = param1 - param2;
			break;
		case '*':
			output = param1 * param2;
			break;
		case '/':
			if(param2 == 0) { //Invalid denominator
				return -1;
			}
			output = param1 / param2;
			break;
		default: //Invalid operator
			return -1;
	}
	//Attempt to copy to result to verify that the address works properly
	if(copy_to_user(result, &output, sizeof(output))) {
		return -EFAULT;
	}
	return 0; //If it makes it to this point all arguments work
}

//Set current version to the default, may be modified later by modules
calc_t calc_curr = calc_default;
EXPORT_SYMBOL(calc_curr);

SYSCALL_DEFINE4(calc, int, param1, int, param2, char, operation, int __user *, result) {
	return calc_curr(param1, param2, operation, result);
}
