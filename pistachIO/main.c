#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <dlfcn.h>

#include "opt.h"
#include "printk.h"
#include "libc.h"
#include "entry/syscall.h"
#include "ipc/ipc.h"
#include "net/dpdk_module.h"
#include "loader/loader.h"
#include "net/net.h"
#include "fs/file.h"
#include "fs/fs.h"
#include "fs/aio.h"
#include "kernel/sched.h"
#include "doca/context.h"

#define MAX_EVENTS  1024

bool kernel_early_boot = true;
bool kernel_shutdown = false;

struct sock_info {
    int parent;
    int sockfd;
};

void signal_handler(int sig) { 
    switch (sig) {
        case SIGINT:
            pr_info("Caught SIGINT! Terminating...\n");
            pr_info("Goodbye!\n");
            exit(1);
            break;
        default:
            break;
    }
}

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

static int handle_read_event(int epfd, struct epoll_event * ev) {
    struct sock_info * info;
    int parent, sockfd;
    char buffer[128] = {0};
    // char reply[128] = {0};
 
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
    } else if (parent == loader_fd) {
        if (recv(sockfd, buffer, sizeof(buffer), 0) < 0) {
            pr_err("Failed to recv message from remote!\n");
            return -1;
        }

        compile_and_run((const char *)buffer);
    }

    return 0;
}

int pistachio_loop(void) {
    int ret, new_sockfd;
    int epfd, nevent;
    struct epoll_event ev, events[MAX_EVENTS];

    /* Register termination handling callback */
    signal(SIGINT, signal_handler); 

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
    ev.data.fd = loader_fd;

    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, loader_fd, &ev);
    if (ret == -1) {
        pr_err("<%s:%d> Failed to register epoll event for loader fd %d! (%s)\n", __func__, __LINE__, loader_fd, strerror(errno));
        return -1;
    }

    while (1) {
        /* Poll epoll IPC events from workers if there is any */
        nevent = epoll_wait(epfd, events, MAX_EVENTS, 0);
        for(int i = 0; i < nevent; i++) {
            /* New worker is connecting to control plane */
            if (events[i].data.fd == control_fd) {
                pr_info("Incoming connection!\n");
                if ((new_sockfd = accept(control_fd, NULL, NULL)) < 0) {
                    pr_err("Failed to accept incoming connection\n");
                    continue;
                }
                if (accept_new_connection(epfd, control_fd, new_sockfd) < 0) {
                    pr_err("Failed to register incoming connection\n");
                }
                continue;
            }

            if (events[i].data.fd == loader_fd) {
                pr_info("Incoming load request!\n");
                if ((new_sockfd = accept(loader_fd, NULL, NULL)) < 0) {
                    pr_err("Failed to accept incoming connection\n");
                    continue;
                }
                if (accept_new_connection(epfd, loader_fd, new_sockfd) < 0) {
                    pr_err("Failed to register incoming connection\n");
                }
                continue;
            }

            if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP) {
                close(events[i].data.fd);
                continue;
            }

            if (events[i].events & EPOLLIN) {
                handle_read_event(epfd, &events[i]);
            }
        }
    }
}

int main(int argc, char ** argv) {
    /* Init DPDK module (packet mempool, RX/TX queue, ...) */
    pr_info("init: starting DPDK...\n");
    dpdk_init(argc, argv);

#ifdef CONFIG_DOCA_REGEX
    pr_info("init: initializing DOCA...\n");
    doca_init();
#endif

    kernel_early_boot = false;

    pr_info("init: initializing fs...\n");
	fs_init();

    /* Reserve 0, 1, and 2 for stdin, stdout, and stderr */
    set_open_fd(0);
    set_open_fd(1);
    set_open_fd(2);

    pr_info("init: initializing worker threads...\n");
    worker_init();

    pr_info("init: initializing network module...\n");
    net_init();

    pr_info("init: register INET domain...\n");
	inet_init();

    pr_info("init: initializing ipc...\n");
	ipc_init();

    pr_info("init: initializing loader...\n");
	loader_init();

    void (*preload_aio_init)(void);
    void (*preload_lnftnl_init)(void);
    char *error;

    /* Preload libaio */
    void* libaio_preload = dlopen("./build/lib/libaio.so", RTLD_NOW | RTLD_GLOBAL);
    if (!libaio_preload) {
        fprintf(stderr, "Failed to load preload library: %s\n", dlerror());
        exit(EXIT_FAILURE);
    }

    if ((error = dlerror()) != NULL)  {
        fprintf(stderr, "%s\n", error);
        exit(EXIT_FAILURE);
    }

    *(void **) (&preload_aio_init) = dlsym(libaio_preload, "io_init");
    assert(preload_aio_init != NULL);
    preload_aio_init();

    /* Preload libnftnl */
    void* libnftnl_preload = dlopen("./build/lib/libnftnl.so", RTLD_NOW | RTLD_GLOBAL);
    if (!libnftnl_preload) {
        fprintf(stderr, "Failed to load preload library: %s\n", dlerror());
        exit(EXIT_FAILURE);
    }

    if ((error = dlerror()) != NULL)  {
        fprintf(stderr, "%s\n", error);
        exit(EXIT_FAILURE);
    }

    *(void **) (&preload_lnftnl_init) = dlsym(libnftnl_preload, "nftnl_init");
    assert(preload_lnftnl_init != NULL);
    preload_lnftnl_init();

#if 0
    // Open the shared library
    handle = dlopen("/local/yihan/Nutcracker-dev/apps/aio_dns_filter/aio_dns_filter.so", RTLD_NOW);
    if (!handle) {
        fprintf(stderr, "%s\n", dlerror());
        exit(EXIT_FAILURE);
    }

    *(void **) (&my_function) = dlsym(handle, "aio_dns_filter_main");

    if ((error = dlerror()) != NULL)  {
        fprintf(stderr, "%s\n", error);
        exit(EXIT_FAILURE);
    }
    
    printf("Func@ %p, io_setup: %p, io_ctx_mp: %p\n", my_function, io_setup, io_ctx_mp);

    // Call the function
    my_function();
#endif

    pr_info("init: initializing LIBC...\n");
    libc_hook_init();

    pr_info("init: initializing SYSCALL...\n");
    syscall_hook_init();

    /* Now enter the main loop */
    pr_info("init: starting pistachIO loop...\n");
    pistachio_loop();

    return 0;
}
