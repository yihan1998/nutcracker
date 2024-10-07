#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "opt.h"
#include "utils/printk.h"
#include "interpose/interpose.h"
#include "fs/fs.h"
#include "fs/file.h"

const struct rte_memzone * skb_mp_lock_mz;
pthread_spinlock_t * skb_mp_lock;
struct rte_mempool * skb_mp;

struct rte_mempool * socket_mp;
struct rte_mempool * sock_mp;
struct rte_mempool * ioctx_mp;
struct rte_mempool * iotask_mp;
struct rte_mempool * nftask_mp;

int __init fs_init(void) {
    skb_mp = rte_mempool_lookup("skb_mp");
    assert(skb_mp != NULL);

    socket_mp = rte_mempool_lookup("socket_mp");
    assert(socket_mp != NULL);

    sock_mp = rte_mempool_lookup("sock_mp");
    assert(sock_mp != NULL);

    file_mp = rte_mempool_lookup("file_mp");
    assert(file_mp != NULL);

    nftask_mp = rte_mempool_lookup("nftask_mp");
    assert(nftask_mp != NULL);

    int fd = shm_open("fdtable", O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    files = mmap(NULL, sizeof(struct files_struct), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (files == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    skb_mp_lock_mz = rte_memzone_lookup("skb_mp_lock");
	if (skb_mp_lock_mz == NULL) {
		pr_err("Failed to reserve memzone\n");
	}
    skb_mp_lock = skb_mp_lock_mz->addr;

    return 0;
}