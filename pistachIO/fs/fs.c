#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "opt.h"
#include "printk.h"
#include "file.h"
#include "fs.h"
#include "net.h"

#include <rte_mempool.h>

#define NR_MAX_SOCKET   2048

int __init fs_init(void) {
    int shm_fd;
    void * ptr;
    struct rte_mempool * mp;

    mp = rte_mempool_create("socket_mp", NR_MAX_SOCKET, sizeof(struct socket), 0, 0, NULL, NULL, NULL, NULL, rte_socket_id(), 0);
    assert(mp != NULL);

    mp = rte_mempool_create("file_mp", NR_MAX_FILE, sizeof(struct file), 0, 0, NULL, NULL, NULL, NULL, rte_socket_id(), 0);
    assert(mp != NULL);

    // Open the shared memory segment
    shm_fd = shm_open("file_table", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 0;
    }

    // Size the shared memory object
    if (ftruncate(shm_fd, sizeof(struct files_struct)) == -1) {
        perror("ftruncate");
        close(shm_fd);
        shm_unlink("file_table");
        return 0;
    }

    // Map the shared memory segment to the process's address space
    ptr = mmap(0, sizeof(struct files_struct), PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    pr_debug(FILE_DEBUG, " > shared pointer: %p\n", ptr);

    files = (struct files_struct *)ptr;
    memset(files, 0, sizeof(struct files_struct));

    /* Reserve 0, 1, and 2 for stdin, stdout, and stderr */
    set_open_fd(0);
    set_open_fd(1);
    set_open_fd(2);

    return 0;
}