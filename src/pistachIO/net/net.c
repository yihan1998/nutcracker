#define _GNU_SOURCE
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <rte_tailq.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "opt.h"
#include "printk.h"
#include "init.h"
#include "net/net.h"
#include "net/flow.h"
#include "net/raw.h"

static struct rte_hash_parameters params = {
    .entries = 2048,
    .key_len = sizeof(struct flowi4),
    .hash_func = rte_jhash,
    .hash_func_init_val = 0,
    .socket_id = 0,
    .extra_flag = RTE_HASH_EXTRA_FLAGS_RW_CONCURRENCY_LF
				| RTE_HASH_EXTRA_FLAGS_MULTI_WRITER_ADD,
};

struct rte_hash * flow_table;

struct rte_tailq_entry_head * pre_routing_table;
struct rte_tailq_entry_head * local_in_table;
struct rte_tailq_entry_head * forward_table;
struct rte_tailq_entry_head * local_out_table;
struct rte_tailq_entry_head * post_routing_table;

pthread_spinlock_t rx_lock;
pthread_spinlock_t tx_lock;

const struct rte_memzone * pending_sk_mz;
struct locked_list_head * pending_sk;

int lock_init(void) {    
    pthread_spin_init(&rx_lock, PTHREAD_PROCESS_SHARED);
    pthread_spin_init(&tx_lock, PTHREAD_PROCESS_SHARED);

    return 0;
}

int __init net_init(void) {
    struct rte_tailq_elem pre_routing_tailq = {
        .name = "pre_routing_table",
    };
    struct rte_tailq_elem local_in_tailq = {
        .name = "local_in_table",
    };
    struct rte_tailq_elem forward_tailq = {
        .name = "forward_table",
    };
    struct rte_tailq_elem local_out_tailq = {
        .name = "local_out_table",
    };
    struct rte_tailq_elem post_routing_tailq = {
        .name = "post_routing_table",
    };

    if ((rte_eal_tailq_register(&pre_routing_tailq) < 0) || (pre_routing_tailq.head == NULL)) {
		pr_err("Error allocating %s\n", pre_routing_tailq.name);
    }
    pre_routing_table = RTE_TAILQ_CAST(pre_routing_tailq.head, rte_tailq_entry_head);
    assert(pre_routing_table != NULL);

    if ((rte_eal_tailq_register(&local_in_tailq) < 0) || (local_in_tailq.head == NULL)) {
		pr_err("Error allocating %s\n", local_in_tailq.name);
    }
    local_in_table = RTE_TAILQ_CAST(local_in_tailq.head, rte_tailq_entry_head);
    assert(local_in_table != NULL);

    if ((rte_eal_tailq_register(&forward_tailq) < 0) || (forward_tailq.head == NULL)) {
		pr_err("Error allocating %s\n", forward_tailq.name);
    }
    forward_table = RTE_TAILQ_CAST(forward_tailq.head, rte_tailq_entry_head);
    assert(forward_table != NULL);

    if ((rte_eal_tailq_register(&local_out_tailq) < 0) || (local_out_tailq.head == NULL)) {
		pr_err("Error allocating %s\n", local_out_tailq.name);
    }
    local_out_table = RTE_TAILQ_CAST(local_out_tailq.head, rte_tailq_entry_head);
    assert(local_out_table != NULL);

    if ((rte_eal_tailq_register(&post_routing_tailq) < 0) || (post_routing_tailq.head == NULL)) {
		pr_err("Error allocating %s\n", post_routing_tailq.name);
    }
    post_routing_table = RTE_TAILQ_CAST(post_routing_tailq.head, rte_tailq_entry_head);
    assert(post_routing_table != NULL);

    params.name = "flow_table";
    flow_table = rte_hash_create(&params);
    assert(flow_table != NULL);

    pending_sk_mz = rte_memzone_reserve_aligned("pending_sk", sizeof(struct locked_list_head), SOCKET_ID_ANY, 0, RTE_CACHE_LINE_SIZE);
	if (pending_sk_mz == NULL) {
		pr_err("Failed to reserve memzone\n");
	}
    
    pending_sk = pending_sk_mz->addr;
    pthread_spin_init(&pending_sk->lock, PTHREAD_PROCESS_SHARED);
    init_list_head(&pending_sk->head);

    lock_init();

    raw_init();

    return 0;
}