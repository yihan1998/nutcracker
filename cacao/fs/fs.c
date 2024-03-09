#include <assert.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "opt.h"
#include "printk.h"
#include "kernel/sched.h"
#include "fs/fs.h"
#include "fs/file.h"
#include "syscall.h"

struct rte_mempool * socket_mp;
struct rte_mempool * file_mp;

struct files_struct * files;

int __init fs_init(void) {
    int shm_fd;
    void * ptr;

    socket_mp = rte_mempool_lookup("socket_mp");
    assert(socket_mp != NULL);

    file_mp = rte_mempool_lookup("file_mp");
    assert(file_mp != NULL);

   // Open the shared memory segment
    shm_fd = shm_open("file_table", O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
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

    return 0;
}