#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "opt.h"
#include "printk.h"
#include "syscall.h"
#include "ipc/ipc.h"

#define IPC_PATH    "/tmp/ipc.socket"

int epfd;
int control_fd;

int __init ipc_init(void) {
    struct sockaddr_un address;
	address.sun_family = AF_UNIX;
	strcpy(address.sun_path, IPC_PATH);

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

    if (chmod(IPC_PATH, 0666) == -1) {
        pr_crit("Failed to change socket privilege!");
        return -1;
    }

    if (libc_listen(control_fd, 20) < 0) {
        pr_crit("Failed to listen socket!\n");
        return -1;
    }

    return 0;
}