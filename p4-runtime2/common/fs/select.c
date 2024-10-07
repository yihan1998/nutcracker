#include "opt.h"
#include "utils/printk.h"
#include "syscall.h"
#include "kernel/sched.h"

static int __select(int nfds, fd_set * readfds, fd_set * writefds, fd_set * exceptfds, struct timeval * timeout) {
    return 0;
}

int select(int nfds, fd_set * readfds, fd_set * writefds, fd_set * exceptfds, struct timeval * timeout) {
    if (unlikely(!syscall_hooked)) {
        syscall_hook_init();
    }

    if (unlikely(kernel_early_boot)) {
        return libc_select(nfds, readfds, writefds, exceptfds, timeout);
    }

    pr_debug(FS_DEBUG, "%s: nfds: %d, readfds: %p, writefds: %p, exceptfds: %p, timeout: %p\n", 
            __func__, nfds, readfds, writefds, exceptfds, timeout);
    return __select(nfds, readfds, writefds, exceptfds, timeout);
}