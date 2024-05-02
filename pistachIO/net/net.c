#define _GNU_SOURCE
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>

#include "opt.h"
#include "printk.h"
#include "kernel.h"
#include "fs/fs.h"
#include "net/dpdk_module.h"
#include "net/net.h"
#include "net/flow.h"
#include "net/sock.h"
#include "net/ethernet.h"
#include "net/tcp.h"
#include "net/udp.h"
#include "linux/netfilter.h"

#ifdef CONFIG_DOCA
#include "doca/context.h"
#endif  /* CONFIG_DOCA */

#include <rte_ethdev.h>
#include <rte_tailq.h>

#define NSEC_PER_USEC       1000L
#define USEC_PER_SEC        1000000L
#define TIMESPEC_TO_USEC(t) ((t.tv_sec * USEC_PER_SEC) + (t.tv_nsec / NSEC_PER_USEC))

struct rte_hash * flow_table;

struct rte_tailq_entry_head * pre_routing_table;
struct rte_tailq_entry_head * local_in_table;
struct rte_tailq_entry_head * forward_table;
struct rte_tailq_entry_head * local_out_table;
struct rte_tailq_entry_head * post_routing_table;

struct net_stats stats;

static struct rte_hash_parameters params = {
	.entries = 2048,
	.key_len = sizeof(struct flowi4),
	.hash_func = rte_jhash,
	.hash_func_init_val = 0,
	.socket_id = 0,
    .extra_flag = RTE_HASH_EXTRA_FLAGS_RW_CONCURRENCY 
                | RTE_HASH_EXTRA_FLAGS_MULTI_WRITER_ADD,
};

pthread_spinlock_t rx_lock;
pthread_spinlock_t tx_lock;

DEFINE_PER_CPU(struct rte_ring *, fwd_queue);

int rxtx_module(void * arg) {
    int pid = 0, nb_recv = 0, nb_send = 0;
    int sec_recv = 0, sec_send = 0;
    // int ret;
    uint8_t * pkt;
    int pkt_size;
    struct sk_buff * skb;
	struct rte_mbuf * m;
    struct timespec curr, last_log, last_poll;
    char name[RTE_MEMZONE_NAMESIZE];
    bool is_main = false;

    cpu_id = sched_getcpu();
#if RTE_VERSION >= RTE_VERSION_NUM(20, 11, 0, 0)
    if (cpu_id == rte_get_main_lcore()) is_main = true;
#else
    if (cpu_id == rte_get_master_lcore()) is_main = true;
#endif
    // worker_ctx = (struct worker_context *)arg;
    worker_ctx = contexts[cpu_id];

    sprintf(name, "fwd_queue_%d", cpu_id);
    fwd_queue = rte_ring_create(name, 4096, rte_socket_id(), 0);
    assert(fwd_queue != NULL);

#ifdef CONFIG_DOCA
    pr_info("Initialze DOCA percore context...\n");
    doca_percore_init(worker_ctx);
#endif  /* CONFIG_DOCA */

    clock_gettime(CLOCK_REALTIME, &last_log);
    curr = last_log;
    last_poll = last_log;

    while (1) {
        // int count = 0;
        // struct sock * sk = NULL;
        struct rte_mbuf * pkts[MAX_PKT_BURST];
        
        clock_gettime(CLOCK_REALTIME, &curr);
        if (curr.tv_sec - last_log.tv_sec >= 1) {
            // pr_info("[RXTX] receive %d packets, send %d packets, pkt mp: %d, skb mp: %d, nftask mp: %d\n", 
            //         sec_recv, sec_send, rte_mempool_avail_count(pkt_mempool), rte_mempool_avail_count(skb_mp), rte_mempool_avail_count(nftask_mp));
            pr_info("[RXTX] receive %d packets, send %d packets\n", sec_recv, sec_send);
            sec_recv = sec_send = 0;
            last_log = curr;
        }

        if (is_main) {
            if (TIMESPEC_TO_USEC(curr) - TIMESPEC_TO_USEC(last_poll) >= 100000) {
                pistachio_control_plane();
                last_poll = curr;
            }
        }

        pthread_spin_lock(&rx_lock);
        nb_recv = dpdk_recv_pkts(pid, pkts);
        pthread_spin_unlock(&rx_lock);
        if (nb_recv) {
            sec_recv += nb_recv;
            for (int i = 0; i < nb_recv; i++) {
                pkt = dpdk_get_rxpkt(pid, pkts, i, &pkt_size);
                // fprintf(stderr, "CPU %02d| PKT %p is received\n", sched_getcpu(), pkt);
                ethernet_input(pkts[i], pkt, pkt_size);
            }
        }
#if 0
        count = rte_ring_count(worker_cq);
        for (int i = 0; i < count; i++) {
            if (rte_ring_dequeue(worker_cq, (void **)&sk) == 0) {
                pr_debug(STACK_DEBUG, "%s: sock protocol: %d\n", __func__, sk->sk_protocol);
                lock_sock(sk);
                switch (sk->sk_protocol) {
                    case IPPROTO_IP:
                    case IPPROTO_TCP:
                        pr_warn("Todo: TCP tx\n");
                        break;
                    case IPPROTO_UDP:
                        udp_output(sk);
                        // sec_count++;
                        break;
                }
                sk->sk_tx_pending = 0;
                unlock_sock(sk);
            }
        }
#endif
        // {
        //     struct nfcb_task_struct * t, * tasks[32];
        //     int nb_recv = rte_ring_dequeue_burst(nf_rq, (void **)tasks, 32, NULL);
        //     if (nb_recv) {
        //         for (int i = 0; i < nb_recv; i++) {
        //             t = tasks[i];
        //             if (t->entry.hook(t->entry.priv, t->skb, NULL) == NF_ACCEPT) {
        //                 if (rte_ring_enqueue(nf_cq, t->skb) < 0) {
        //                     // pr_debug(NF_DEBUG, "Failed to enqueue into fwd CQ\n");
        //                     rte_pktmbuf_free(t->skb->m);
        //                     free_skb(t->skb);
        //                 }
        //             }
        //         }
        //         rte_mempool_put_bulk(nftask_mp, (void *)tasks, nb_recv);
        //     }
        // }

        int ret, nb_recv;
        struct sk_buff * skbs[32];

        nb_recv = rte_ring_dequeue_burst(fwd_queue, (void **)skbs, 32, NULL);
        if (nb_recv) {
            for (int i = 0; i < nb_recv; i++) {
                skb = skbs[i];
                m = skb->m;
                // pthread_spin_lock(&tx_lock);
                ret = dpdk_insert_txpkt(pid, m);
                // pthread_spin_unlock(&tx_lock);
                if (ret < 0) {
                    rte_pktmbuf_free(m);
                }
            }
    		rte_mempool_put_bulk(skb_mp, (void *)skbs, nb_recv);
        }

        nb_recv = rte_ring_dequeue_burst(nf_cq, (void **)skbs, 32, NULL);
        if (nb_recv) {
            for (int i = 0; i < nb_recv; i++) {
                skb = skbs[i];
                m = skb->m;
                // pthread_spin_lock(&tx_lock);
                ret = dpdk_insert_txpkt(pid, m);
                // pthread_spin_unlock(&tx_lock);
                if (ret < 0) {
                    rte_pktmbuf_free(m);
                }
            }
    		rte_mempool_put_bulk(skb_mp, (void *)skbs, nb_recv);
        }

        pthread_spin_lock(&tx_lock);
        nb_send = dpdk_send_pkts(pid);
        pthread_spin_unlock(&tx_lock);
        if (nb_send) {
            sec_send += nb_send;
            pr_debug(STACK_DEBUG, "%s: send %d packets\n", __func__, nb_send);
        }
    }

    return 0;
}

int __init net_init(void) {
    // pthread_t rxtx_pid;
    // pthread_attr_t attr;
    // cpu_set_t cpuset;

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

    params.name = "flow_table";
    flow_table = rte_hash_create(&params);
    assert(flow_table != NULL);

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

    // Initialize the thread attribute
    // pthread_attr_init(&attr);

    pthread_spin_init(&rx_lock, PTHREAD_PROCESS_SHARED);
    pthread_spin_init(&tx_lock, PTHREAD_PROCESS_SHARED);

//     // Create the RX path
//     for (int i = 0; i < nr_rxtx_module; i++) {
//         struct worker_context * ctx = (struct worker_context *)calloc(1, sizeof(struct worker_context));
// #ifdef CONFIG_DOCA
//         if (doca_worker_init(ctx) != DOCA_SUCCESS) {
//             pr_err("Failed to create work queue for core %d\n", i);
//         }
// #endif

//         // Initialize the CPU set to be empty
//         CPU_ZERO(&cpuset);
//         // RX core runs on core 0
//         CPU_SET(i, &cpuset);

//         // Set the CPU affinity in the thread attributes
//         if (pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset) != 0) {
//             perror("pthread_attr_setaffinity_np");
//             exit(EXIT_FAILURE);
//         }

//         // if (pthread_create(&rx_pid, &attr, rx_module, NULL) != 0) {
//         if (pthread_create(&rxtx_pid, &attr, rxtx_module, ctx) != 0) {
//             perror("Create RX path failed!");
//             exit(EXIT_FAILURE);
//         }
//     }
#if 0
    uint32_t lcore_id = 0;

    RTE_LCORE_FOREACH_WORKER(lcore_id) {
        struct worker_context * ctx = (struct worker_context *)calloc(1, sizeof(struct worker_context));
#ifdef CONFIG_DOCA
        if (doca_worker_init(ctx) != DOCA_SUCCESS) {
            pr_err("Failed to create work queue for core %d\n", lcore_id);
        }
#endif
        rte_eal_remote_launch(rxtx_module, ctx, lcore_id);
    }
#endif
    return 0;
}