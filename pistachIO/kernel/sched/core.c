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
#include "net/net.h"
#include "net/skbuff.h"
#include "net/dpdk_module.h"
#include "fs/fs.h"

#define MAX_WORK_BURST  32

unsigned int nr_cpu_ids = NR_CPUS;

pthread_t worker_ids[NR_CPUS];

DEFINE_PER_CPU(unsigned int, cpu_id);

struct rte_ring * worker_rq;
struct rte_ring * worker_cq;
struct rte_ring * nf_rq;
struct rte_ring * nf_cq;

struct rte_mempool * task_mp;

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

void * worker_main(void * arg) {
    int nb_recv = 0;
    worker_ctx = (struct worker_context *)arg;
#ifdef CONFIG_DOCA
    pr_info("Initialze DOCA percore context...\n");
    doca_percore_init(worker_ctx);
#endif  /* CONFIG_DOCA */
    // int sec_count = 0;
    // struct timespec curr, last_log;

    // clock_gettime(CLOCK_REALTIME, &last_log);
    // curr = last_log;

    while(1) {
        // struct iocb_task_struct tasks[MAX_WORK_BURST];
        struct nfcb_task_struct * t, * tasks[MAX_WORK_BURST];

        // clock_gettime(CLOCK_REALTIME, &curr);
        // if (curr.tv_sec - last_log.tv_sec >= 1) {
        //     pr_info("Finished: %d\n", sec_count);
        //     sec_count = 0;
        //     last_log = curr;
        // }

        // while (rte_ring_count(worker_rq) != 0) {
        //     if (rte_ring_dequeue(worker_rq, (void **)&iocb_task) == 0) {
        //         void * argp = iocb_task->argp;
        //         void (*func)(void *) = iocb_task->func;
        //         rte_mempool_put(iotask_mp, iocb_task);
        //         func(argp);
        //         // sec_count++;
        //     }
        // }

        // work_cnt = rte_ring_dequeue_burst(worker_rq, (void **)tmp_pkts, MAX_WORK_BURST, NULL);
        nb_recv = rte_ring_dequeue_burst(nf_rq, (void **)tasks, MAX_WORK_BURST, NULL);
        if (nb_recv) {
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
            }
    		rte_mempool_put_bulk(nftask_mp, (void *)tasks, nb_recv);
        }
    }
    return NULL;
}

int __init worker_init(void) {
    pthread_attr_t attr;
    cpu_set_t cpuset;

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

    // Initialize the thread attribute
    pthread_attr_init(&attr);

    /* Create one runtime thread on each possible core */
    // for (int i = 0; i < nr_cpu_ids; i++) {
    for (int i = 0; i < nr_cpu_ids - NR_RXTX_MODULE; i++) {
        CPU_ZERO(&cpuset);
        CPU_SET(i + NR_RXTX_MODULE, &cpuset);

        struct worker_context * ctx = (struct worker_context *)calloc(1, sizeof(struct worker_context));
#ifdef CONFIG_DOCA
        if (doca_worker_init(ctx) != DOCA_SUCCESS) {
            pr_err("Failed to create work queue for core %d\n", i);
        }
#endif
        // Set the CPU affinity in the thread attributes
        if (pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset) != 0) {
            perror("pthread_attr_setaffinity_np");
            exit(EXIT_FAILURE);
        }

        // Create the RX path
        if (pthread_create(&worker_ids[i], &attr, worker_main, ctx) != 0) {
            perror("Create RX path failed!");
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}