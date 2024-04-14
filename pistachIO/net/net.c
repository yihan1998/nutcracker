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

#include <rte_ethdev.h>
#include <rte_tailq.h>

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

void * rxtx_module(void * arg) {
    int pid = 0, nb_recv = 0, nb_send = 0;
    int sec_recv = 0, sec_send = 0;
    int ret;
    uint8_t * pkt;
    int pkt_size;
    struct sk_buff * skb;
    struct timespec curr, last_log;

    clock_gettime(CLOCK_REALTIME, &last_log);
    curr = last_log;

    while (1) {
        int count = 0;
        // struct sock * sk = NULL;
        struct rte_mbuf * pkts[MAX_PKT_BURST];
        
        clock_gettime(CLOCK_REALTIME, &curr);
        if (curr.tv_sec - last_log.tv_sec >= 1) {
            pr_info("[RXTX] receive %d packets, send %d packets\n", sec_recv, sec_send);
            sec_recv = sec_send = 0;
            last_log = curr;
        }

        pthread_spin_lock(&rx_lock);
        nb_recv = dpdk_recv_pkts(pid, pkts);
        pthread_spin_unlock(&rx_lock);
        if (nb_recv) {
            sec_recv += nb_recv;
            for (int i = 0; i < nb_recv; i++) {
                pkt = dpdk_get_rxpkt(pid, pkts, i, &pkt_size);
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
        count = rte_ring_count(fwd_cq);
        for (int i = 0; i < count; i++) {
            if (rte_ring_dequeue(fwd_cq, (void **)&skb) == 0) {
                pthread_spin_lock(&tx_lock);
                ret = dpdk_insert_txpkt(pid, skb->m);
                pthread_spin_unlock(&tx_lock);
                if (ret < 0) {
                    rte_ring_enqueue(fwd_cq, skb);
                    break;
                }
                free_skb(skb);
            }
        }

        pthread_spin_lock(&tx_lock);
        nb_send = dpdk_send_pkts(pid);
        pthread_spin_unlock(&tx_lock);
        if (nb_send) {
            sec_send += nb_send;
            pr_debug(STACK_DEBUG, "%s: send %d packets\n", __func__, nb_send);
        }
    }

    return NULL;
}

void * rx_module(void * arg) {
    int pid = 0, nb_recv;
    uint8_t * pkt;
    int pkt_size;
    int sec_count = 0;
    struct timespec curr, last_log;

    clock_gettime(CLOCK_REALTIME, &last_log);
    curr = last_log;

    while (1) {
        struct rte_mbuf * pkts[MAX_PKT_BURST];
        
        clock_gettime(CLOCK_REALTIME, &curr);
        if (curr.tv_sec - last_log.tv_sec >= 1) {
            pr_info("[RX] receive %d packets\n", sec_count);
            sec_count = 0;
            last_log = curr;
        }

        pthread_spin_lock(&rx_lock);
        nb_recv = dpdk_recv_pkts(pid, pkts);
        pthread_spin_unlock(&rx_lock);
        if (!nb_recv) {
            continue;
        }

        sec_count += nb_recv;
        for (int i = 0; i < nb_recv; i++) {
            pkt = dpdk_get_rxpkt(pid, pkts, i, &pkt_size);
            ethernet_input(pkts[i], pkt, pkt_size);
            // sec_count++;
        }

        pthread_spin_lock(&rx_lock);
        dpdk_recv_done(pkts, nb_recv);
        pthread_spin_unlock(&rx_lock);
    }

    return NULL;
}

void * tx_module(void * arg) {
    int pid = 0;
    int nb_send = 0;
    struct sk_buff * skb;
	struct rte_mbuf * m;
	// uint8_t * pkt;
    int sec_count = 0;
    struct timespec curr, last_log;

    clock_gettime(CLOCK_REALTIME, &last_log);
    curr = last_log;

    while (1) {
        int count = 0;
        struct sock * sk = NULL;

        clock_gettime(CLOCK_REALTIME, &curr);
        if (curr.tv_sec - last_log.tv_sec >= 1) {
            pr_info("[TX] send %d packets\n", sec_count);
            sec_count = 0;
            last_log = curr;
        }

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

        // count = rte_ring_count(fwd_cq);
        // for (int i = 0; i < count; i++) {
        //     if (rte_ring_dequeue(fwd_cq, (void **)&skb) == 0) {
        //         m = dpdk_alloc_txpkt(skb->len);
        //         if (!m) {
        //             break;
        //         }
        //         pkt = rte_pktmbuf_mtod(m, uint8_t *);
		//         memcpy(pkt, skb->ptr, skb->len);
        //         pthread_spin_lock(&tx_lock);
        //         dpdk_insert_txpkt(pid, m);
        //         pthread_spin_unlock(&tx_lock);
        // 		free_skb(skb);
        //     }
        // }

        int ret;
        struct sk_buff * skbs[32];
        int nb_recv = rte_ring_dequeue_burst(fwd_cq, (void **)skbs, 32, NULL);
        if (nb_recv) {
            for (int i = 0; i < nb_recv; i++) {
                skb = skbs[i];
                m = skb->m;
                pthread_spin_lock(&tx_lock);
                ret = dpdk_insert_txpkt(pid, m);
                pthread_spin_unlock(&tx_lock);
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
            sec_count += nb_send;
            pr_debug(STACK_DEBUG, "%s: send %d packets\n", __func__, nb_send);
        }
    }

    return NULL;
}
#if 0
int __init net_init(void) {
    pthread_t rx_pid;
    pthread_t tx_pid;
    pthread_attr_t attr;
    cpu_set_t cpuset;
    int nr_rx_module = 1;
    int nr_tx_module = 0;

    fwd_cq = rte_ring_create("fwd_cq", 1024, rte_socket_id(), 0);
    assert(fwd_cq != NULL);

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
    pthread_attr_init(&attr);

    pthread_spin_init(&rx_lock, PTHREAD_PROCESS_SHARED);
    pthread_spin_init(&tx_lock, PTHREAD_PROCESS_SHARED);

    // Create the RX path
    for (int i = 0; i < nr_rx_module; i++) {
        // Initialize the CPU set to be empty
        CPU_ZERO(&cpuset);
        // RX core runs on core 0
        CPU_SET(i, &cpuset);

        // Set the CPU affinity in the thread attributes
        if (pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset) != 0) {
            perror("pthread_attr_setaffinity_np");
            exit(EXIT_FAILURE);
        }

        // if (pthread_create(&rx_pid, &attr, rx_module, NULL) != 0) {
        if (pthread_create(&rx_pid, &attr, rxtx_module, NULL) != 0) {
            perror("Create RX path failed!");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < nr_tx_module; i++) {
        // Initialize the CPU set to be empty
        CPU_ZERO(&cpuset);
        // TX core runs on core 1
        CPU_SET(nr_rx_module + i, &cpuset);

        // Set the CPU affinity in the thread attributes
        if (pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset) != 0) {
            perror("pthread_attr_setaffinity_np");
            exit(EXIT_FAILURE);
        }

        // Create the TX path
        if (pthread_create(&tx_pid, &attr, tx_module, NULL) != 0) {
            perror("Create TX path failed!");
            exit(EXIT_FAILURE);
        }
    }

    // Destroy the thread attributes object, as it's no longer needed
    pthread_attr_destroy(&attr);

    // // Wait for the thread to complete
    // if (pthread_join(rx_pid, NULL) != 0) {
    //     perror("RX join");
    //     exit(EXIT_FAILURE);
    // }

    // // Wait for the thread to complete
    // if (pthread_join(tx_pid, NULL) != 0) {
    //     perror("TX join");
    //     exit(EXIT_FAILURE);
    // }

    return 0;
}
#endif
int __init net_init(void) {
    pthread_t rxtx_pid;
    pthread_attr_t attr;
    cpu_set_t cpuset;
    int nr_rxtx_module = 4;

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
    pthread_attr_init(&attr);

    pthread_spin_init(&rx_lock, PTHREAD_PROCESS_SHARED);
    pthread_spin_init(&tx_lock, PTHREAD_PROCESS_SHARED);

    // Create the RX path
    for (int i = 0; i < nr_rxtx_module; i++) {
        // Initialize the CPU set to be empty
        CPU_ZERO(&cpuset);
        // RX core runs on core 0
        CPU_SET(i, &cpuset);

        // Set the CPU affinity in the thread attributes
        if (pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset) != 0) {
            perror("pthread_attr_setaffinity_np");
            exit(EXIT_FAILURE);
        }

        // if (pthread_create(&rx_pid, &attr, rx_module, NULL) != 0) {
        if (pthread_create(&rxtx_pid, &attr, rxtx_module, NULL) != 0) {
            perror("Create RX path failed!");
            exit(EXIT_FAILURE);
        }
    }

    // // Wait for the thread to complete
    // if (pthread_join(rx_pid, NULL) != 0) {
    //     perror("RX join");
    //     exit(EXIT_FAILURE);
    // }

    // // Wait for the thread to complete
    // if (pthread_join(tx_pid, NULL) != 0) {
    //     perror("TX join");
    //     exit(EXIT_FAILURE);
    // }

    return 0;
}