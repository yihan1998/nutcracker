#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <time.h>

#include "init.h"

extern bool syscall_hooked;

extern int __init syscall_hook_init(void);

extern int (*libc_socket)(int domain, int type, int protocol);
extern int (*libc_connect)(int fd, const struct sockaddr * addr, int addrlen);
extern int (*libc_accept4)(int sockfd, struct sockaddr * addr, socklen_t * addrlen, int flags);
extern ssize_t (*libc_sendmsg)(int fd, const struct msghdr * msg, int flags);
extern ssize_t (*libc_sendto)(int fd, const void * buf, size_t len, int flags, const struct sockaddr * addr, socklen_t addrlen);
extern ssize_t (*libc_send)(int fd, const void * buf, size_t len, int flags);
extern ssize_t (*libc_recvmsg)(int fd, struct msghdr * msg, int flags);
extern ssize_t (*libc_recvfrom)(int fd, void * buf, size_t len, int flags, struct sockaddr * addr, socklen_t * addrlen);
extern ssize_t (*libc_recv)(int fd, void * buf, size_t len, int flags);
extern int (*libc_bind)(int fd, const struct sockaddr * addr, socklen_t addrlen);
extern int (*libc_getpeername)(int sockfd, struct sockaddr * addr, socklen_t * addrlen);
extern void (*libc_exit)(int status);
extern int (*libc_listen)(int sockfd, int backlog);

extern int (*libc_ioctl)(int fd, unsigned long int request, ...);

extern int (*libc_select)(int nfds, fd_set * readfds, fd_set * writefds, fd_set * exceptfds, struct timeval * timeout);

// extern long syscall(long number, ...);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* _SYSCALL_H_ */