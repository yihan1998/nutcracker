#define _GNU_SOURCE
#define __USE_GNU
#include <dlfcn.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/socket.h>

#include "init.h"
#include "printk.h"

bool syscall_hooked = false;

int (*libc_socket)(int domain, int type, int protocol) = NULL;
int (*libc_connect)(int fd, const struct sockaddr * addr, int addrlen) = NULL;
int (*libc_accept4)(int sockfd, struct sockaddr * addr, socklen_t * addrlen, int flags) = NULL;
ssize_t (*libc_sendmsg)(int fd, const struct msghdr * msg, int flags) = NULL;
ssize_t (*libc_sendto)(int fd, const void * buf, size_t len, int flags, const struct sockaddr * addr, socklen_t addrlen) = NULL;
ssize_t (*libc_send)(int fd, const void * buf, size_t len, int flags) = NULL;
ssize_t (*libc_recvmsg)(int fd, struct msghdr * msg, int flags) = NULL;
ssize_t (*libc_recvfrom)(int fd, void * buf, size_t len, int flags, struct sockaddr * addr, socklen_t * addrlen) = NULL;
ssize_t (*libc_recv)(int fd, void * buf, size_t len, int flags) = NULL;
int (*libc_bind)(int fd, const struct sockaddr * addr, socklen_t addrlen) = NULL;
int (*libc_getpeername)(int sockfd, struct sockaddr * addr, socklen_t * addrlen) = NULL;
void (*libc_exit)(int status) = NULL;
int (*libc_listen)(int sockfd, int backlog) = NULL;

static void * bind_symbol(const char * sym) {
    void * ptr;
    
    if ((ptr = dlsym(RTLD_NEXT, sym)) == NULL) {
        pr_err("Libc interpose: dlsym failed (%s)\n", sym);
    }

    return ptr;
}

int __init syscall_hook_init(void) {
    libc_socket = bind_symbol("socket");
    libc_connect = bind_symbol("connect");
    libc_accept4 = bind_symbol("accept4");
    libc_sendmsg = bind_symbol("sendmsg");
    libc_sendto = bind_symbol("sendto");
    libc_send = bind_symbol("send");
    libc_recvmsg = bind_symbol("recvmsg");
    libc_recvfrom = bind_symbol("recvfrom");
    libc_recv = bind_symbol("recv");
    libc_bind = bind_symbol("bind");
    libc_getpeername = bind_symbol("getpeername");
    libc_exit = bind_symbol("exit");
    libc_listen = bind_symbol("listen");

    syscall_hooked = true;

    return 0;
}