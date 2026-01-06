# Real Time Task Reservations in the RPI Kernel
## Background
This project was done for Professor Hyunjong Choi's CS596: Real Time Systems course. It involves modifying the Raspberry Pi kernel to implement support for reservations for real time tasks.

## Branches
The project is broken up into four parts, each part tackling a slightly different problem/task.
- main: This is the unmodified rpi-kernel-4.9.80 with no changes.
- project1: This is the basic project setup, getting acquainted with making minor changes like modifying the version name and recompiling the kernel.
- project2: This is where we got aquainted with the concept of loadable kernel modules. We made system calls for a calculator app with modules that would "hack" the program, replacing all the functions with a modulo operator.
- project3: Project3 is where we start to get into the fun stuff. We made a dummy task in the userspace with a calibration function so that it would still work off of the latencies of different CPUs, and implemented 3 kernel system calls for managing real time scheduling for real time tasks: set_rsv(), cancel_rsv(), and wait_until_next_period();
- project4: Project4 is where I spent the majority of my time, a blogpost detailing my anguish can be found [here](https://cameron-lee.com/blog/real-time-linux-scheduler). In this final part, we modified part 3 to support partitioned earliest deadline first scheduling instead of monotonic. We also added support for chain information and measuring end-to-end latency for said chains.
