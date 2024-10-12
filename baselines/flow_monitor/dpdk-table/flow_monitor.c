
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <sys/time.h>
#include <setjmp.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <regex.h>
#include <linux/if_ether.h>
#include <linux/udp.h>

#include <rte_common.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_eal.h>
#include <rte_launch.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_string_fns.h>
#include <rte_ip.h>
#include <rte_hash.h>
#include <rte_fbk_hash.h>
#include <rte_jhash.h>
#include <rte_hash_crc.h>

#include "list.h"

struct vxlanhdr {
    uint8_t  flags;
    uint8_t  reserved1[3];
    uint32_t vni_reserved2;
} __attribute__((__packed__));

#define USEC_PER_SEC    1000000L
#define TIMEVAL_TO_USEC(t)  ((t.tv_sec * USEC_PER_SEC) + t.tv_usec)

#define DEFAULT_PKT_BURST   32
#define DEFAULT_RX_DESC     4096
#define DEFAULT_TX_DESC     4096

int nb_cores;

__thread struct rte_ring * fwd_queue;

pthread_rwlock_t fsm_lock;

struct match_action_table_entry {
    struct list_head list;
    bool (*cond_check)(struct rte_mbuf * m, uint8_t * pkt, int pkt_size);
    int (*callback_fn)(struct rte_mbuf * m, uint8_t * pkt, int pkt_size);
};

struct list_head match_action_table;

static struct rte_hash_parameters params = {
    .entries = 2048,
    .key_len = ETH_ALEN,
    .hash_func = rte_jhash,
    .hash_func_init_val = 0,
    .socket_id = 0,
    .extra_flag = RTE_HASH_EXTRA_FLAGS_RW_CONCURRENCY_LF
				| RTE_HASH_EXTRA_FLAGS_MULTI_WRITER_ADD,
};
struct rte_hash * flow_table;

/* RX queue configuration */
static struct rte_eth_rxconf rx_conf = {
    .rx_thresh = {
        .pthresh = 8,
        .hthresh = 8,
        .wthresh = 4,
    },
    .rx_free_thresh = 32,
};

/* TX queue configuration */
static struct rte_eth_txconf tx_conf = {
    .tx_thresh = {
        .pthresh = 36,
        .hthresh = 0,
        .wthresh = 0,
    },
    .tx_free_thresh = 0,
};

/* Port configuration */
struct rte_eth_conf port_conf = {
    .rxmode = {
        .mq_mode = RTE_ETH_MQ_RX_RSS,
    },
    .txmode = {
        .mq_mode = RTE_ETH_MQ_TX_NONE,
        .offloads = (RTE_ETH_TX_OFFLOAD_IPV4_CKSUM |
                RTE_ETH_TX_OFFLOAD_UDP_CKSUM |
                RTE_ETH_TX_OFFLOAD_TCP_CKSUM),
    },
    .rx_adv_conf = {
        .rss_conf = {
            .rss_key = NULL,
            .rss_hf =
                RTE_ETH_RSS_IP | RTE_ETH_RSS_TCP | RTE_ETH_RSS_UDP,
        },
    },
};

/* Packet mempool for each core */
struct rte_mempool * pkt_mempools[8];

static int config_ports(void) {
    int ret;
    uint16_t portid;
    char name[RTE_MEMZONE_NAMESIZE];
    uint16_t nb_rxd = DEFAULT_RX_DESC;
    uint16_t nb_txd = DEFAULT_TX_DESC;

    for (int i = 0; i < nb_cores; i++) {
        /* Create mbuf pool for each core */
        sprintf(name, "mbuf_pool_%d", i);
        pkt_mempools[i] = rte_pktmbuf_pool_create(name, 8192,
            RTE_MEMPOOL_CACHE_MAX_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE,
            rte_socket_id());
        if (pkt_mempools[i] == NULL) {
            rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");
        } else {
            printf("MBUF pool %u: %p...\n", i, pkt_mempools[i]);
        }
    }

    /* Initialise each port */
	RTE_ETH_FOREACH_DEV(portid) {
		struct rte_eth_dev_info dev_info;

        printf("Initializing port %u...", portid);
		fflush(stdout);

		ret = rte_eth_dev_info_get(portid, &dev_info);
		if (ret != 0) {
			rte_exit(EXIT_FAILURE, "Error during getting device (port %u) info: %s\n", portid, strerror(-ret));
        }

		/* Configure the number of queues for a port. */
		ret = rte_eth_dev_configure(portid, nb_cores, nb_cores, &port_conf);
		if (ret < 0) {
			rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%u\n", ret, portid);
        }
		/* >8 End of configuration of the number of queues for a port. */

        for (int i = 0; i < nb_cores; i++) {
            /* RX queue setup. 8< */
            ret = rte_eth_rx_queue_setup(portid, i, nb_rxd, rte_eth_dev_socket_id(portid), &rx_conf, pkt_mempools[i]);
            if (ret < 0) {
                rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup:err=%d, port=%u\n", ret, portid);
            }

            ret = rte_eth_tx_queue_setup(portid, i, nb_txd, rte_eth_dev_socket_id(portid), &tx_conf);
            if (ret < 0) {
                rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup:err=%d, port=%u\n", ret, portid);
            }
        }
		/* >8 End of queue setup. */

		fflush(stdout);

		/* Start device */
		ret = rte_eth_dev_start(portid);
		if (ret < 0) {
			rte_exit(EXIT_FAILURE, "rte_eth_dev_start:err=%d, port=%u\n", ret, portid);
        }

		printf("done\n");
        ret = rte_eth_promiscuous_enable(portid);
        if (ret != 0) {
            rte_exit(EXIT_FAILURE, "rte_eth_promiscuous_enable:err=%s, port=%u\n", rte_strerror(-ret), portid);
        }
    }

    return 0;
}

void print_ether_hdr(struct rte_ether_hdr * ethhdr) {
    printf("Ethernet Header:\n");
    printf("  Destination MAC " RTE_ETHER_ADDR_PRT_FMT "\n", RTE_ETHER_ADDR_BYTES(&ethhdr->dst_addr));
    printf("  Source MAC " RTE_ETHER_ADDR_PRT_FMT "\n", RTE_ETHER_ADDR_BYTES(&ethhdr->src_addr));
    printf("  EtherType: 0x%04x\n", ntohs(ethhdr->ether_type));
}

#define SET_MAC_ADDR(addr, a, b, c, d, e, f)\
do {\
	addr[0] = a & 0xff;\
	addr[1] = b & 0xff;\
	addr[2] = c & 0xff;\
	addr[3] = d & 0xff;\
	addr[4] = e & 0xff;\
	addr[5] = f & 0xff;\
} while (0)									/* create source mac address */


bool cond_check(struct rte_mbuf * m, uint8_t * pkt, int pkt_size) {
    return true;
}

int callback_fn(struct rte_mbuf * m, uint8_t * pkt, int pkt_size) {
    struct ethhdr *ethhdr;
    ethhdr = (struct ethhdr *)pkt;

	int ret;
	uint32_t *data;
	ret = rte_hash_lookup_data(flow_table, (const void *) ethhdr->h_dest, (void **)&data);
	if (ret >= 0) {
		(*data)++;
		ret = rte_hash_add_key_data(flow_table, (const void *) ethhdr->h_dest, data);
        if (ret < 0) {
			printf("Update failed for flow table\n");
		}
        // printf("DST MAC: %02x:%02x:%02x:%02x:%02x:%02x, Counter %d\n", 
        //     ethhdr->h_dest[0], ethhdr->h_dest[1], ethhdr->h_dest[2],
        //     ethhdr->h_dest[3], ethhdr->h_dest[4], ethhdr->h_dest[5], (*data));
	} else {
        uint32_t * counter = (uint32_t *)calloc(1, sizeof(uint32_t));
        *counter = 0;
		ret = rte_hash_add_key_data(flow_table, (const void *) ethhdr->h_dest, counter);
        if (ret < 0) {
            printf("Insertion failed for flow table\n");
		}
        // printf("Add new DST MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", 
        //     ethhdr->h_dest[0], ethhdr->h_dest[1], ethhdr->h_dest[2],
        //     ethhdr->h_dest[3], ethhdr->h_dest[4], ethhdr->h_dest[5]);
    }

    return 0;
}

int check_match_action_table(struct rte_mbuf * m, uint8_t * pkt, int pkt_size) {
    pthread_rwlock_rdlock(&fsm_lock);
    struct match_action_table_entry * entry;
    list_for_each_entry(entry, &match_action_table, list) {
        if (entry->cond_check) {
            entry->callback_fn(m, pkt, pkt_size);
        }
    }
    pthread_rwlock_unlock(&fsm_lock); 
    return 0;
}

int ethernet_input(struct rte_mbuf * m, uint8_t * pkt, int pkt_size) {
    return check_match_action_table(m, pkt, pkt_size);
}

static int launch_one_lcore(void * args) {
    int nb_rx, nb_tx;
	uint32_t lid, qid;
    struct rte_mbuf * rx_pkts[DEFAULT_PKT_BURST];
    struct timeval log, curr;
    uint32_t sec_nb_rx, sec_nb_tx;

    lid = rte_lcore_id();
    qid = lid;

    sec_nb_rx = sec_nb_tx = 0;

    gettimeofday(&log, NULL);

    while (1) {
        gettimeofday(&curr, NULL);
        if (curr.tv_sec - log.tv_sec >= 1) {
            printf("CPU %02d| rx: %u/%4.2f (MPS), tx: %u/%4.2f (MPS)\n", rte_lcore_id(), 
                sec_nb_rx, ((double)sec_nb_rx) / (TIMEVAL_TO_USEC(curr) - TIMEVAL_TO_USEC(log)),
                sec_nb_tx, ((double)sec_nb_tx) / (TIMEVAL_TO_USEC(curr) - TIMEVAL_TO_USEC(log)));
            sec_nb_rx = sec_nb_tx = 0;
            gettimeofday(&log, NULL);
        }
        nb_rx = rte_eth_rx_burst(0, qid, rx_pkts, DEFAULT_PKT_BURST);
        if (nb_rx) {
            sec_nb_rx += nb_rx;

            for (int i = 0; i < nb_rx; i++) {
                struct rte_mbuf * rx_pkt = rx_pkts[i];
                int pkt_size = rx_pkt->pkt_len;
                uint8_t * pkt = rte_pktmbuf_mtod(rx_pkt, uint8_t *);
                ethernet_input(rx_pkt, pkt, pkt_size);
            }

            nb_tx = rte_eth_tx_burst(0, qid, rx_pkts, nb_rx);
            sec_nb_tx += nb_tx;
            if (unlikely(nb_tx < nb_rx)) {
                do {
                    rte_pktmbuf_free(rx_pkts[nb_tx]);
                } while (++nb_tx < nb_rx);
            }
        }

    }
    return 0;
}

int main(int argc, char ** argv) {
	int ret;
    if ((ret = rte_eal_init(argc, argv)) < 0) {
		rte_exit(EXIT_FAILURE, "Cannot init EAL: %s\n", rte_strerror(rte_errno));
    }

    argc -= ret;
	argv += ret;

    nb_cores = rte_lcore_count();

    config_ports();

    params.name = "flow_monitor_tbl";
    flow_table = rte_hash_create(&params);
    assert(flow_table != NULL);

    pthread_rwlockattr_t attr;
    pthread_rwlockattr_init(&attr);
    pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    if (pthread_rwlock_init(&fsm_lock, &attr) != 0) {
        perror("Failed to initialize rwlock");
    }

    init_list_head(&match_action_table);

    struct match_action_table_entry * entry = (struct match_action_table_entry *)calloc(1, sizeof(struct match_action_table_entry));
    entry->cond_check = cond_check;
    entry->callback_fn = callback_fn;
    list_add_tail(&entry->list, &match_action_table);

    rte_eal_mp_remote_launch(launch_one_lcore, NULL, CALL_MAIN);
    rte_eal_mp_wait_lcore();

    /* clean up the EAL */
	rte_eal_cleanup();
	printf("Bye...\n");

    return 0;
}
