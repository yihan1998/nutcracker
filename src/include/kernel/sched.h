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
#include "doca/context.h"
#endif  /* CONFIG_DOCA */

extern bool kernel_early_boot;
extern bool kernel_shutdown;

enum Role {
    IDLE = 1,
    RXTX,
    WORKER,
};

DECLARE_PER_CPU(enum Role, role);

struct worker_context {
#ifdef CONFIG_DOCA
    struct docadv_worker_context docadv_ctx;
#endif  /* CONFIG_DOCA */
};

extern struct worker_context contexts[NR_CPUS];
DECLARE_PER_CPU(struct worker_context *, worker_ctx);

extern int flax_worker_ctx_fetch(int core_id);

/* Used in tsk->state: */
#define TASK_RUNNING			0x0000
#define TASK_INTERRUPTIBLE		0x0001
#define TASK_UNINTERRUPTIBLE		0x0002
#define __TASK_STOPPED			0x0004
#define __TASK_TRACED			0x0008
/* Used in tsk->exit_state: */
#define EXIT_DEAD			0x0010
#define EXIT_ZOMBIE			0x0020
#define EXIT_TRACE			(EXIT_ZOMBIE | EXIT_DEAD)
/* Used in tsk->state again: */
#define TASK_PARKED			0x0040
#define TASK_DEAD			0x0080
#define TASK_WAKEKILL			0x0100
#define TASK_WAKING			0x0200
#define TASK_NOLOAD			0x0400
#define TASK_NEW			0x0800
#define TASK_STATE_MAX			0x1000

#define TASK_NORMAL (TASK_INTERRUPTIBLE | TASK_UNINTERRUPTIBLE)

struct task_struct {
    void    * argp;
    void    (*func)(void *);
};

extern struct task_struct * create_new_task(void * argp, void (*func)(void *));

extern struct rte_ring * worker_rq;
extern struct rte_ring * worker_cq;

extern struct rte_ring * nf_rq;
extern struct rte_ring * nf_cq;

DECLARE_PER_CPU(struct rte_ring *, fwd_queue);
extern void * worker_main(void);

extern uint64_t get_current_time_ns(void);

extern int pistachio_control_plane(void);
extern int __init sched_init(void);

#endif  /* _KERNEL_SCHED_H_ */