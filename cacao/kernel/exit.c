#include <stdnoreturn.h>

#include "percpu.h"
#include "printk.h"
#include "syscall.h"
#include "kernel/util.h"

DEFINE_PER_CPU(unsigned long, exit_stack[1024]);

void noreturn do_exit(int status) {
    // do_task_dead();
    __builtin_unreachable();
}

void noreturn __do_exit(int status) {
	do_exit((status&0xff)<<8);
}

void noreturn exit(int status) {
    // if (unlikely(kernel_shutdown)) {
    //     libc_exit(status);
    // }
    // INVOKE_SYSCALL(1, NR_exit, status);
    // entry_syscall();
    __builtin_unreachable();
}