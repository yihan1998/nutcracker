#define _GNU_SOURCE
#define __USE_GNU
#include <dlfcn.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/socket.h>

#include "init.h"
#include "utils/printk.h"
#include "syscall.h"
// #include "kernel/threads.h"
// #include "kernel/sched.h"
// #include "fs/fs.h"

bool syscall_hooked = false;

int (*libc_socket)(int domain, int type, int protocol) = NULL;
int (*libc_accept4)(int sockfd, struct sockaddr * addr, socklen_t * addrlen, int flags) = NULL;
int (*libc_bind)(int fd, const struct sockaddr * addr, socklen_t addrlen) = NULL;
int (*libc_listen)(int sockfd, int backlog) = NULL;
ssize_t (*libc_sendto)(int fd, const void * buf, size_t len, int flags, const struct sockaddr * addr, socklen_t addrlen) = NULL;
ssize_t (*libc_sendmsg)(int fd, const struct msghdr * message, int flags) = NULL;
ssize_t (*libc_recvfrom)(int fd, void * buf, size_t len, int flags, struct sockaddr * addr, socklen_t * addrlen) = NULL;
ssize_t (*libc_recvmsg)(int fd, struct msghdr * message, int flags) = NULL;

int (*libc_ioctl)(int fd, unsigned long int request, ...) = NULL;

int (*libc_select)(int nfds, fd_set * readfds, fd_set * writefds, fd_set * restrict exceptfds, struct timeval * timeout) = NULL;

static void * bind_symbol(const char * sym) {
    void * ptr;

    if ((ptr = dlsym(RTLD_NEXT, sym)) == NULL) {
        pr_err("Libc interpose: dlsym failed (%s)\n", sym);
    }

    return ptr;
}

int __init syscall_hook_init(void) {
    pr_info("INIT: initialize syscall hook...\n");

    libc_socket = bind_symbol("socket");
    libc_accept4 = bind_symbol("accept4");
    libc_bind = bind_symbol("bind");
    libc_listen = bind_symbol("listen");
    libc_sendmsg = bind_symbol("sendmsg");
    libc_sendto = bind_symbol("sendto");
    libc_recvmsg = bind_symbol("recvmsg");
    libc_recvfrom = bind_symbol("recvfrom");

    libc_ioctl = bind_symbol("ioctl");

    syscall_hooked = true;

    return 0;
}