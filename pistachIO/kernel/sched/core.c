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
#include "fs/fs.h"

#define MAX_WORK_BURST  32

unsigned int nr_cpu_ids = NR_CPUS;

pthread_t worker_ids[NR_CPUS];

struct rte_ring * worker_rq;
struct rte_ring * worker_cq;
struct rte_ring * fwd_rq;
struct rte_ring * fwd_cq;

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
        nb_recv = rte_ring_dequeue_burst(fwd_rq, (void **)tasks, MAX_WORK_BURST, NULL);
        if (nb_recv) {
            for (int i = 0; i < nb_recv; i++) {
                t = tasks[i];
                if (t->entry.hook(t->entry.priv, t->skb, NULL) == NF_ACCEPT) {
                    while (rte_ring_enqueue(fwd_cq, t->skb) < 0);
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

    fwd_rq = rte_ring_create("fwd_rq", 4096, rte_socket_id(), 0);
    assert(fwd_rq != NULL);

    fwd_cq = rte_ring_create("fwd_cq", 4096, rte_socket_id(), 0);
    assert(fwd_cq != NULL);

    // task_mp = rte_mempool_create("task_mp", 8192, sizeof(struct task_struct), 0, 0, NULL, NULL, NULL, NULL, rte_socket_id(), 0);
    // assert(task_mp != NULL);

    // Initialize the thread attribute
    pthread_attr_init(&attr);

    /* Create one runtime thread on each possible core */
    // for (int i = 0; i < nr_cpu_ids; i++) {
    for (int i = 0; i < nr_cpu_ids - 4; i++) {
        CPU_ZERO(&cpuset);
        CPU_SET(i + 4, &cpuset);
        // Set the CPU affinity in the thread attributes
        if (pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset) != 0) {
            perror("pthread_attr_setaffinity_np");
            exit(EXIT_FAILURE);
        }

        // Create the RX path
        if (pthread_create(&worker_ids[i], &attr, worker_main, NULL) != 0) {
            perror("Create RX path failed!");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}