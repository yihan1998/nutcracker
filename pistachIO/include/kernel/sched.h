#ifndef _KERNEL_SCHED_H_
#define _KERNEL_SCHED_H_

#include <stdbool.h>
#include <unistd.h>
#include <sched.h>

#include "init.h"
#include "list.h"

#include <rte_mempool.h>

extern bool kernel_early_boot;
extern bool kernel_shutdown;

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

extern struct rte_ring * fwd_cq;

extern int __init sched_init(void);

#endif  /* _KERNEL_SCHED_H_ */