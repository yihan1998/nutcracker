#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "opt.h"
#include "printk.h"
#include "syscall.h"
#include "ipc/ipc.h"

#define MAX_EVENTS  1024

#define COMPILER_PATH   "/tmp/compiler.socket"
#define CONTROL_PATH    "/tmp/control.socket"

int epfd;
int compiler_fd;    /* For connecting to compiler */
int control_fd;     /* For connecting to control plane */

struct sock_info {
    int parent;
    int sockfd;
};

static int accept_new_connection(int epfd, int parent, int new_sockfd) {
    int ret, flags;
    struct epoll_event ev = {0};
    struct sock_info * info;

    flags = fcntl(new_sockfd, F_GETFL);

    pr_info("Accept incoming connection via sock %d!\n", new_sockfd);

    if (fcntl(new_sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        pr_err("<%s:%d> Failed to set new socket to non-blocking mode...\n", __func__, __LINE__);
        return -1;
    }

    info = (struct sock_info *)calloc(1, sizeof(struct sock_info));
    info->parent = parent;
    info->sockfd = new_sockfd;

    ev.events = EPOLLIN;
    ev.data.ptr = info;

    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, new_sockfd, &ev);
    if (ret == -1) {
        pr_err("<%s:%d> Failed to register epoll event! (%s)\n", __func__, __LINE__, strerror(errno));
        return -1;
    }

    return 0;
}

static int handle_epollin_event(int epfd, struct epoll_event * ev) {
    struct sock_info * info;
    int parent, sockfd;
    char buffer[128] = {0};
 
    info = (struct sock_info *)ev->data.ptr;
    parent = info->parent;
    sockfd = info->sockfd;

    pr_debug(IPC_DEBUG, "Handle read event on sockfd %d\n", sockfd);

    if (parent == control_fd) {
        if (recv(sockfd, buffer, sizeof(buffer), 0) < 0) {
            pr_err("Failed to recv message from remote!\n");
            return -1;
        }

        pr_debug(IPC_DEBUG, "Receive %s from remote\n", buffer);
    } else if (parent == compiler_fd) {
        if (recv(sockfd, buffer, sizeof(buffer), 0) < 0) {
            pr_err("Failed to recv message from remote!\n");
            return -1;
        }

        cacao_attach_and_run((const char *)buffer);
    }

    return 0;
}

int ipc_poll(void) {
    int new_sockfd, nevent;
    struct epoll_event events[MAX_EVENTS];

    /* Poll epoll IPC events from workers if there is any */
    nevent = epoll_wait(epfd, events, MAX_EVENTS, 0);
    for(int i = 0; i < nevent; i++) {
        /* New worker is connecting to control plane */
        if (events[i].data.fd == control_fd) {
            pr_info("Incoming connection!\n");
            if ((new_sockfd = libc_accept4(control_fd, NULL, NULL, 0)) < 0) {
                pr_err("Failed to accept incoming connection\n");
                continue;
            }
            if (accept_new_connection(epfd, control_fd, new_sockfd) < 0) {
                pr_err("Failed to register incoming connection\n");
            }
            continue;
        }

        if (events[i].data.fd == compiler_fd) {
            pr_info("Incoming load request!\n");
            if ((new_sockfd = libc_accept4(compiler_fd, NULL, NULL, 0)) < 0) {
                pr_err("Failed to accept incoming connection\n");
                continue;
            }
            if (accept_new_connection(epfd, compiler_fd, new_sockfd) < 0) {
                pr_err("Failed to register incoming connection\n");
            }
            continue;
        }

        if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP) {
            close(events[i].data.fd);
            continue;
        }

        if (events[i].events & EPOLLIN) {
            handle_epollin_event(epfd, &events[i]);
        }
    }

    return 0;
}

int __init compiler_init(void) {
    struct sockaddr_un address;
	address.sun_family = AF_UNIX;
	strcpy(address.sun_path, COMPILER_PATH);

    if (remove(address.sun_path) == -1 && errno != ENOENT) {
        pr_crit("Failed remove existing socket file!\n");
        return -1;
    }

    if ((compiler_fd = libc_socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        pr_crit("Failed to create socket for IPC!\n");
        return -1;
    }

    int flags = fcntl(compiler_fd, F_GETFL);
    if (fcntl(compiler_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        pr_crit("Failed to set IPC socket to non-blocking mode!\n");
        return -1;
    }

    if (libc_bind(compiler_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        pr_crit("Failed to bind socket!\n");
        return -1;
    }

    if (chmod(COMPILER_PATH, 0666) == -1) {
        pr_crit("Failed to change socket privilege!");
        return -1;
    }

    if (libc_listen(compiler_fd, 20) < 0) {
        pr_crit("Failed to listen socket!\n");
        return -1;
    }

    return 0;
}

int __init control_init(void) {
    struct sockaddr_un address;
	address.sun_family = AF_UNIX;
	strcpy(address.sun_path, CONTROL_PATH);

    if (remove(address.sun_path) == -1 && errno != ENOENT) {
        pr_crit("Failed remove existing socket file!\n");
        return -1;
    }

    if ((control_fd = libc_socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        pr_crit("Failed to create socket for IPC!\n");
        return -1;
    }

    int flags = fcntl(control_fd, F_GETFL);
    if (fcntl(control_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        pr_crit("Failed to set IPC socket to non-blocking mode!\n");
        return -1;
    }

    if (libc_bind(control_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        pr_crit("Failed to bind socket!\n");
        return -1;
    }

    if (chmod(CONTROL_PATH, 0666) == -1) {
        pr_crit("Failed to change socket privilege!");
        return -1;
    }

    if (libc_listen(control_fd, 20) < 0) {
        pr_crit("Failed to listen socket!\n");
        return -1;
    }

    return 0;
}

int __init ipc_init(void) {
    int ret;
    struct epoll_event ev;

    pr_info("\tinit: initializing control...\n");
    control_init();

    pr_info("\tinit: initializing compiler...\n");
    compiler_init();

    /* Create epoll file descriptor */
    epfd = epoll_create1(0);

    if(epfd == -1) {
        pr_err("Failed to create epoll fd in control plane!\n");
        return -1;
    }

    /* Register EPOLLIN event for control fd */
    ev.events = EPOLLIN;
    ev.data.fd = control_fd;

    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, control_fd, &ev);
    if (ret == -1) {
        pr_err("<%s:%d> Failed to register epoll event for control fd %d! (%s)\n", __func__, __LINE__, control_fd, strerror(errno));
        return -1;
    }

    /* Register EPOLLIN event for loader fd */
    ev.events = EPOLLIN;
    ev.data.fd = compiler_fd;

    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, compiler_fd, &ev);
    if (ret == -1) {
        pr_err("<%s:%d> Failed to register epoll event for compiler fd %d! (%s)\n", __func__, __LINE__, compiler_fd, strerror(errno));
        return -1;
    }

    return 0;
}