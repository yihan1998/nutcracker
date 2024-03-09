#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <stdlib.h>

#include "opt.h"
#include "printk.h"
#include "ipc.h"
#include "core.h"

int control_fd;

int __init ipc_init(void) {
    struct sockaddr_un address;
	address.sun_family = AF_UNIX;
	strcpy(address.sun_path, "/tmp/ipc.socket");

    if (remove(address.sun_path) == -1 && errno != ENOENT) {
        pr_crit("Failed remove existing socket file!\n");
        return -1;
    }

    if ((control_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        pr_crit("Failed to create socket for IPC!\n");
        return -1;
    }

    int flags = fcntl(control_fd, F_GETFL);
    if (fcntl(control_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        pr_crit("Failed to set IPC socket to non-blocking mode!\n");
        return -1;
    }

    if (bind(control_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        pr_crit("Failed to bind socket!\n");
        return -1;
    }

    if (listen(control_fd, 20) < 0) {
        pr_crit("Failed to listen socket!\n");
        return -1;
    }

    return 0;
}