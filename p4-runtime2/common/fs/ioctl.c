#include "opt.h"
#include "utils/printk.h"
#include "syscall.h"
#include "kernel/sched.h"
#include "fs/fs.h"

int __ioctl(unsigned int fd, unsigned int cmd, unsigned long arg) {
    return 0;
}

int ioctl(unsigned int fd, unsigned int cmd, unsigned long arg) {
    if (unlikely(!syscall_hooked)) {
        syscall_hook_init();
    }

    if (unlikely(kernel_early_boot)) {
        return libc_ioctl(fd, cmd, arg);
    }

    pr_debug(FS_DEBUG, "%s: fd: %d, cmd: %u, arg: %lu\n", __func__, fd, cmd, arg);
    return __ioctl(fd, cmd, arg);
}