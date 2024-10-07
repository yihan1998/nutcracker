#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <rte_mempool.h>

#include "opt.h"
#include "utils/printk.h"
#include "interpose/interpose.h"

int control_fd;
struct rte_ring * recv_queue;
struct rte_ring * send_queue;

int __init ipc_init(void) {
    struct sockaddr_un address;
    char command[64] = "CONNECT_PROCESS";
    char res[128] = {0};

    address.sun_family = AF_UNIX;
	strcpy(address.sun_path, "/tmp/control.socket");

    if ((control_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        pr_err("Failed to create socket for IPC!(errno: %d)\n", errno);
        return -1;
    }

    if (connect(control_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        pr_err("Failed to connect to control plane!(errno: %d)\n", errno);
        return -1;
    }

    if (write(control_fd, command, strlen(command)) == -1) {
        pr_err("Failed to send absolute path to control plane\n");
        return -1;
    }

    if (read(control_fd, res, strlen(res)) == -1) {
        pr_info("Failed to receive reply from control plane\n", res);
    }

    /* Need some time for the ring to create */
    sleep(2);

    recv_queue = rte_ring_lookup("test_sq");
    assert(recv_queue != NULL);
    send_queue = rte_ring_lookup("test_rq");
    assert(send_queue != NULL);

    return 0;
}