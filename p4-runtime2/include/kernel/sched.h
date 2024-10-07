#ifndef _KERNEL_SCHED_H_
#define _KERNEL_SCHED_H_

#include <stdbool.h>
#include <unistd.h>
#include <sched.h>

#include "init.h"
#include "list.h"
#include "percpu.h"
#include "kernel/threads.h"

#include <rte_mempool.h>

#ifdef CONFIG_DOCA
#include "drivers/doca/context.h"
#endif  /* CONFIG_DOCA */

extern bool kernel_early_boot;
extern bool kernel_shutdown;

struct lcore_context {
#ifdef CONFIG_DOCA
    struct docadv_context docadv_ctx;
#endif  /* CONFIG_DOCA */
};

extern struct lcore_context lcore_ctxs[NR_CPUS];
DECLARE_PER_CPU(struct lcore_context *, lcore_ctx);

extern struct rte_ring * worker_rq;
extern struct rte_ring * worker_cq;
extern struct rte_ring * nf_rq;
extern struct rte_ring * nf_cq;

DECLARE_PER_CPU(struct rte_ring *, fwd_queue);

extern void * worker_main(void);
extern int __init sched_init(void);

#endif  /* _KERNEL_SCHED_H_ */