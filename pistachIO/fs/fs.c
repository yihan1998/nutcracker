#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "net/net.h"
#include "net/skbuff.h"
#include "linux/netfilter.h"
#include "fs/fs.h"
#include "fs/file.h"
#include "fs/aio.h"

#define NR_MAX_SOCKET   4096

struct rte_mempool * skb_mp;
struct rte_mempool * socket_mp;
struct rte_mempool * ioctx_mp;
struct rte_mempool * iotask_mp;
struct rte_mempool * nftask_mp;

int __init fs_init(void) {
    int fd;

    skb_mp = rte_mempool_create("skb_mp", 32768, sizeof(struct sk_buff) + 1460, RTE_MEMPOOL_CACHE_MAX_SIZE, 0, NULL, NULL, NULL, NULL, rte_socket_id(), 0);
    assert(skb_mp != NULL);

    socket_mp = rte_mempool_create("socket_mp", NR_MAX_SOCKET, sizeof(struct socket), RTE_MEMPOOL_CACHE_MAX_SIZE, 0, NULL, NULL, NULL, NULL, rte_socket_id(), 0);
    assert(socket_mp != NULL);

    file_mp = rte_mempool_create("file_mp", NR_MAX_FILE, sizeof(struct file), RTE_MEMPOOL_CACHE_MAX_SIZE, 0, NULL, NULL, NULL, NULL, rte_socket_id(), 0);
    assert(file_mp != NULL);

    ioctx_mp = rte_mempool_create("io_ctx_mp", NR_MAX_IO_CTX, sizeof(struct io_context), RTE_MEMPOOL_CACHE_MAX_SIZE, 0, NULL, NULL, NULL, NULL, rte_socket_id(), 0);
    assert(ioctx_mp != NULL);

    iotask_mp = rte_mempool_create("io_task_mp", NR_MAX_IO_TASK, sizeof(struct iocb_task_struct), RTE_MEMPOOL_CACHE_MAX_SIZE, 0, NULL, NULL, NULL, NULL, rte_socket_id(), 0);
    assert(iotask_mp != NULL);

    nftask_mp = rte_mempool_create("nftask_mp", 32768, sizeof(struct nfcb_task_struct), RTE_MEMPOOL_CACHE_MAX_SIZE, 0, NULL, NULL, NULL, NULL, rte_socket_id(), 0);
    assert(nftask_mp != NULL);

    fd = shm_open("fdtable", O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, sizeof(struct files_struct)) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    struct files_struct * fs = mmap(NULL, sizeof(struct files_struct), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fs == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    memset(fs, 0, sizeof(struct files_struct));

    files = fs;

    return 0;
}