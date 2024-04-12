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

#include "opt.h"
#include "printk.h"

int loader_fd;

int main(int argc, char * argv[]) {
    struct sockaddr_un address;
    char * prog = NULL, prog_path[PATH_MAX];
    char res[128] = {0};

    // Check for correct argument usage
    if (argc != 2) {
        pr_err("Usage: %s [FILE]\n", argv[0]);
        return 1;
    }

    address.sun_family = AF_UNIX;
	strcpy(address.sun_path, "/tmp/loader.socket");

    if ((loader_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        pr_err("Failed to create socket for IPC!(errno: %d)\n", errno);
        return -1;
    }

    pr_debug(IPC_DEBUG, "create ipc socket: %d\n", loader_fd);

    if (connect(loader_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        pr_err("Failed to connect to control plane!(errno: %d)\n", errno);
        return -1;
    }

    pr_debug(IPC_DEBUG, "connected to control plane!\n");

    prog = argv[1];
    // Resolve the absolute path of the file
    if (realpath(prog, prog_path) == NULL) {
        pr_err("Failed to resolve the absolute path of the program\n");
        return -1;
    }

    pr_debug(IPC_DEBUG, "Absolute path: %s\n", prog_path);

    if (write(loader_fd, prog_path, strlen(prog_path)) == -1) {
        pr_err("Failed to send absolute path to control plane");
        return -1;
    }

    sleep(15);

    if (read(loader_fd, res, strlen(res)) == -1) {
        pr_info(" > %s\n", res);
    }

    close(loader_fd);

    return 0;
}