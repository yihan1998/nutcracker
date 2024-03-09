#define _GNU_SOURCE
#define __USE_GNU
#include <sched.h>
#include <pthread.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <fcntl.h>

#include "bitops.h"
#include "ipc.h"
#include "core.h"
#include "printk.h"

struct core_info core_infos[NR_CPUS];
unsigned int cpu_id;

int sched_accept_worker(int epfd) {
    int new_sock, flags;
    struct epoll_event ev;

    if ((new_sock = accept(control_fd, NULL, NULL)) < 0) {
        return -1;
    }

    flags = fcntl(new_sock, F_GETFL);
    if (fcntl(new_sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        pr_warn("Failed to set new socket to non-blocking mode...\n");
        return -1;
    }

    ev.events = EPOLLIN;
    ev.data.fd = new_sock;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, new_sock, &ev) < 0) {
        pr_warn("Failed to set new socket to non-blocking mode...\n");
        return -1;
    }

    return 0;
}

bool try_get_core(int sockfd, int core) {
    if (core_infos[core].occupied) {
        return false;
    }

    core_infos[core].sockfd = sockfd;
    core_infos[core].occupied = true;

    return true;
}

uint64_t get_avail_core(int sockfd, int nr_cores) {
    uint64_t cpumask = 0x0;
    int nr_allocated = 0;

    for (int i = 0; i < NR_CPUS && nr_allocated < nr_cores; i++) {
        if (try_get_core(sockfd, i)) {
            set_bit(i, (void *)&cpumask);
            nr_allocated++;
        }
    }

    return cpumask;
}

int __init sched_init(void) {
    /* Clear all cpu infos */
    memset(core_infos, 0, NR_CPUS * sizeof(struct core_info));

    /* First core is always allocated to control plane */
    core_infos[0].occupied = true;

    cpu_id = sched_getcpu();

    return 0;
}