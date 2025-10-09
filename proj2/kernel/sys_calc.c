#include <linux/syscalls.h> //To modify system calls
#include <linux/kernel.h> //Not sure how necessary this is but every example seems to have it
#include <linux/uaccess.h> //for pointer references

SYSCALL_DEFINE4(calc, int, param1, int, param2, char, operation, int __user *, result) {
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
