#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>

#include "opt.h"
#include "printk.h"
#include "dpdk_module.h"
#include "flow.h"
#include "ethernet.h"
#include "net.h"

#include <rte_ethdev.h>

struct rte_hash * global_udp_table;

static struct rte_hash_parameters params = {
	.entries = 2048,
	.key_len = sizeof(struct flowi4),
	.hash_func = rte_jhash,
	.hash_func_init_val = 0,
	.socket_id = 0,
};

void * rx_module(void * arg) {
    // int pid, nb_recv;
    // uint8_t * pkt;
    // int pkt_size;

    while (1) {
        sleep(1);
        // RTE_ETH_FOREACH_DEV(pid) {
        //     nb_recv = dpdk_recv_pkts(pid);
        //     for (int i = 0; i < nb_recv; i++) {
        //         pkt = dpdk_get_rxpkt(pid, i, &pkt_size);
        //         ethernet_input(pkt, pkt_size);
        //     }
        // }
    }

    return NULL;
}

void * tx_module(void * arg) {
    while (1) {
        sleep(1);
    }

    return NULL;
}

int __init net_init(void) {
    pthread_t rx_pid, tx_pid;
    pthread_attr_t attr;
    cpu_set_t cpuset;

    params.name = "udp_table";
    udp_table = rte_hash_create(&params);
    assert(udp_table != NULL);

    // Initialize the thread attribute
    pthread_attr_init(&attr);

    // Initialize the CPU set to be empty
    CPU_ZERO(&cpuset);
    // RX core runs on core 0
    CPU_SET(0, &cpuset);

    // Set the CPU affinity in the thread attributes
    if (pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset) != 0) {
        perror("pthread_attr_setaffinity_np");
        exit(EXIT_FAILURE);
    }

    // Create the RX path
    if (pthread_create(&rx_pid, &attr, rx_module, NULL) != 0) {
        perror("Create RX path failed!");
        exit(EXIT_FAILURE);
    }

    // Initialize the CPU set to be empty
    CPU_ZERO(&cpuset);
    // TX core runs on core 1
    CPU_SET(1, &cpuset);

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

    // Destroy the thread attributes object, as it's no longer needed
    pthread_attr_destroy(&attr);

    // Wait for the thread to complete
    if (pthread_join(rx_pid, NULL) != 0) {
        perror("RX join");
        exit(EXIT_FAILURE);
    }

    // Wait for the thread to complete
    if (pthread_join(tx_pid, NULL) != 0) {
        perror("TX join");
        exit(EXIT_FAILURE);
    }

    return 0;
}
