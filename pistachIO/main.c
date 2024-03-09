#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h> 

#include "dpdk_module.h"
#include "printk.h"
#include "core.h"
#include "net.h"
#include "ipc.h"
#include "fs.h"

#define MAX_EVENTS  1024

unsigned int nr_cpu_ids = NR_CPUS;

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

int pistachio_loop(void) {
    int epfd, nevent;
    struct epoll_event events[MAX_EVENTS];

    /* Create epoll file descriptor */
    epfd = epoll_create1(0);

    /* Register termination handling callback */
    signal(SIGINT, signal_handler); 

    while (1) {
        /* Poll epoll IPC events from workers if there is any */
        nevent = epoll_wait(epfd, events, MAX_EVENTS, 0);
        for(int i = 0; i < nevent; i++) {
            /* New worker is connecting to control plane */
            if (events[i].data.fd == control_fd) {
                if (sched_accept_worker(epfd) < 0) {
                    /* Failed to accept new worker */
                    pr_warn("Failed to accept new worker!\n");
                    continue;
                }
            }
        }
    }
}

int main(int argc, char ** argv) {
    /* Init DPDK module (packet mempool, RX/TX queue, ...) */
    pr_info("init: starting DPDK...\n");
    dpdk_init(argc, argv);

    /* Init CPU allocation status */
    pr_info("init: initializing CPU status...\n");
    sched_init();

    pr_info("init: initializing fs...\n");
	fs_init();

    pr_info("init: initializing network module...\n");
    net_init();

    /* Now enter the main loop */
    pr_info("init: starting pistachIO loop...\n");
    pistachio_loop();

    return 0;
}
