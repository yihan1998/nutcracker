#define _GNU_SOURCE
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>

#include "kernel/sched.h"
#include "kernel/cpumask.h"

unsigned int nr_cpu_ids = NR_CPUS;

pthread_t worker_ids[NR_CPUS];

void * worker_main(void);

DEFINE_PER_CPU(unsigned int, cpu_id);

struct rte_ring * worker_rq;
struct rte_ring * worker_cq;
struct rte_ring * nf_rq;
struct rte_ring * nf_cq;

struct rte_mempool * task_mp;

struct lcore_context lcore_ctxs[NR_CPUS];
DEFINE_PER_CPU(struct lcore_context *, lcore_ctx);

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