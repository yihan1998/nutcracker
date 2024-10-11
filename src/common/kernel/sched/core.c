#define _GNU_SOURCE
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>

#include "opt.h"
#include "printk.h"
#include "iocontext.h"
#include "kernel.h"
#include "linux/netfilter.h"
#include "kernel/cpumask.h"
#include "kernel/threads.h"
#include "kernel/sched.h"
#include "net/netns/netfilter.h"
#include "fs/fs.h"

#ifdef CONFIG_DOCA
#include "doca/common.h"
#include "doca/context.h"
#endif  /* CONFIG_DOCA */

#define MAX_WORK_BURST  32

unsigned int nr_cpu_ids = NR_CPUS;

pthread_t worker_ids[NR_CPUS];

void * worker_main(void);

DEFINE_PER_CPU(unsigned int, cpu_id);

struct rte_ring * worker_rq;
struct rte_ring * worker_cq;
struct rte_ring * nf_rq;
struct rte_ring * nf_cq;

struct rte_mempool * task_mp;

struct worker_context contexts[NR_CPUS];
DEFINE_PER_CPU(struct worker_context *, worker_ctx);

uint64_t get_current_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

struct task_struct * create_new_task(void * argp, void (*func)(void *)) {
    struct task_struct * new = NULL;

    rte_mempool_get(task_mp, (void **)&new);
    if (!new) {
        pr_err("Failed to allocate new task!\n");
        return NULL;
    }

    new->argp = argp;
    new->func = func;    

    return new;
}

void * worker_main(void) {
    int nb_recv;
    struct nfcb_task_struct * t, * tasks[MAX_WORK_BURST];

    nb_recv = rte_ring_dequeue_burst(nf_rq, (void **)tasks, MAX_WORK_BURST, NULL);
    if (nb_recv) {
        pr_info("[WORKER] Dequeued %d tasks\n", nb_recv);
        for (int i = 0; i < nb_recv; i++) {
            t = tasks[i];
            if (t->entry.hook(t->entry.priv, t->skb, NULL) == NF_ACCEPT) {
                // while (rte_ring_enqueue(nf_cq, t->skb) < 0);
                while (rte_ring_enqueue(nf_cq, t->skb) < 0) {
                    printf("Failed to enqueue into fwq CQ\n");
                    // rte_pktmbuf_free(t->skb->m);
                    // rte_mempool_put(skb_mp, t->skb);
                }
            }
#if 0
            switch (t->entry.type) {
                case LIB_CALLBACK:
                    if (invoke_lib_hook(&t->entry, t->skb) == NF_ACCEPT) {
                        while (rte_ring_enqueue(nf_cq, t->skb) < 0) {
                            printf("Failed to enqueue into fwq CQ\n");
                        }
                    }
                case PYTHON_CALLBACK:
                    printf("Perform Python callback...\n");
                    PyGILState_STATE gstate;
                    gstate = PyGILState_Ensure();
                    PyObject *pBuf = PyBytes_FromStringAndSize((const char *)t->skb->ptr, t->skb->len);
                    if (invoke_python_hook(&t->entry, pBuf) == NF_ACCEPT) {
                        while (rte_ring_enqueue(nf_cq, t->skb) < 0) {
                            printf("Failed to enqueue into fwq CQ\n");
                        }
                    }
                    PyGILState_Release(gstate);
                default:
                    break;
            }
#endif
        }
        rte_mempool_put_bulk(nftask_mp, (void *)tasks, nb_recv);
    }

    return 0;
}

int __init worker_init(void) {
    worker_rq = rte_ring_create("worker_rq", 4096, rte_socket_id(), 0);
    assert(worker_rq != NULL);

    worker_cq = rte_ring_create("worker_cq", 4096, rte_socket_id(), 0);
    assert(worker_cq != NULL);

    nf_rq = rte_ring_create("nf_rq", 8192, rte_socket_id(), 0);
    assert(nf_rq != NULL);

    nf_cq = rte_ring_create("nf_cq", 8192, rte_socket_id(), 0);
    assert(nf_cq != NULL);

    // task_mp = rte_mempool_create("task_mp", 8192, sizeof(struct task_struct), 0, 0, NULL, NULL, NULL, NULL, rte_socket_id(), 0);
    // assert(task_mp != NULL);
    return 0;
}