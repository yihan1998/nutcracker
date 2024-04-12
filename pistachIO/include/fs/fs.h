#ifndef _FS_H_
#define _FS_H_

#include "init.h"
#include "kernel/cpumask.h"

#include <rte_mempool.h>

extern struct rte_mempool * skb_mp;
extern struct rte_mempool * socket_mp;
extern struct rte_mempool * file_mp;
extern struct rte_mempool * ioctx_mp;
extern struct rte_mempool * iotask_mp;

#define NR_FD_BLOCK 40
#define NR_MAX_FILE (NR_FD_BLOCK * BITS_PER_TYPE(unsigned long))

struct files_struct {
	// spinlock_t file_lock;
    DECLARE_BITMAP(bits, NR_MAX_FILE);
    struct file * fdtab[NR_MAX_FILE];
};

extern struct files_struct * files;

extern int __init fs_init(void);

#endif  /* _FS_H_ */