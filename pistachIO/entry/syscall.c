#define _GNU_SOURCE
#define __USE_GNU
#include <dlfcn.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/socket.h>

#include "init.h"
#include "printk.h"
#include "syscall.h"
#include "kernel/sched.h"
#include "fs/fs.h"

bool syscall_hooked = false;

int (*libc_socket)(int domain, int type, int protocol) = NULL;
int (*libc_bind)(int fd, const struct sockaddr * addr, socklen_t addrlen) = NULL;
int (*libc_listen)(int sockfd, int backlog) = NULL;
ssize_t (*libc_sendto)(int fd, const void * buf, size_t len, int flags, const struct sockaddr * addr, socklen_t addrlen) = NULL;
ssize_t (*libc_recvfrom)(int fd, void * buf, size_t len, int flags, struct sockaddr * addr, socklen_t * addrlen) = NULL;

static void * bind_symbol(const char * sym) {
    void * ptr;

    if ((ptr = dlsym(RTLD_NEXT, sym)) == NULL) {
        pr_err("Libc interpose: dlsym failed (%s)\n", sym);
    }

    return ptr;
}

int __init syscall_hook_init(void) {
    libc_socket = bind_symbol("socket");
    libc_bind = bind_symbol("bind");
    libc_listen = bind_symbol("listen");
    libc_sendto = bind_symbol("sendto");
    libc_recvfrom = bind_symbol("recvfrom");

    syscall_hooked = true;

    return 0;
}