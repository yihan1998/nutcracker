
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
    uint16_t udp_dest;
    bool (*cond_check)(struct rte_mbuf * m, uint8_t * pkt, int pkt_size, uint16_t udp_dest);
    uint8_t h_source[8];
    uint8_t h_dest[8];
    uint32_t saddr;
    uint32_t daddr;
    uint16_t source;
    uint16_t dest;
    uint32_t vni;
    int (*callback_fn)(struct rte_mbuf * m, uint8_t * pkt, int pkt_size, uint8_t h_dest[6], uint8_t h_source[6], uint32_t saddr, uint32_t daddr, uint16_t source, uint16_t dest, uint32_t vni);
};

struct list_head match_action_table;

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
struct rte_mempool * pkt_mempools[16];

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

bool cond_check(struct rte_mbuf * m, uint8_t * pkt, int pkt_size, uint16_t dest) {
    struct ethhdr * ethhdr = (struct ethhdr *)pkt;
    struct iphdr * iphdr = (struct iphdr *)&ethhdr[1];
    struct udphdr * udphdr = (struct udphdr *)&iphdr[1];

    if (udphdr->dest == ntohs(dest)) return true;
    return false;
}

int callback_fn(struct rte_mbuf * m, uint8_t * pkt, int pkt_size, uint8_t h_dest[6], uint8_t h_source[6], uint32_t saddr, uint32_t daddr, uint16_t source, uint16_t dest, uint32_t vni) {
    struct ethhdr *orig_ethhdr, *new_ethhdr;
    struct iphdr *orig_iphdr, *new_iphdr;
    struct udphdr *orig_udphdr, *new_udphdr;
    struct vxlanhdr *vxlanhdr;

    orig_ethhdr = (struct ethhdr *)pkt;
    orig_iphdr = (struct iphdr *)&orig_ethhdr[1];
    orig_udphdr = (struct udphdr *)&orig_iphdr[1];

    if (rte_pktmbuf_headroom(m) < (sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr) + sizeof(struct vxlanhdr))) {
        return -1;
    }

    uint8_t *new_pkt = (uint8_t *)rte_pktmbuf_prepend(m, sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr) + sizeof(struct vxlanhdr));
    if (new_pkt == NULL) {
        return -1;
    }

    memmove(new_pkt + sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr) + sizeof(struct vxlanhdr), pkt, pkt_size);

    new_ethhdr = (struct ethhdr *)new_pkt;
    // new_ethhdr = orig_ethhdr;
    new_iphdr = (struct iphdr *)&new_ethhdr[1];
    new_udphdr = (struct udphdr *)&new_iphdr[1];
    vxlanhdr = (struct vxlanhdr *)&new_udphdr[1];

    new_ethhdr->h_proto = orig_ethhdr->h_proto;
    SET_MAC_ADDR(new_ethhdr->h_source,h_source[0],h_source[1],h_source[2],h_source[3],h_source[4],h_source[5]);
    SET_MAC_ADDR(new_ethhdr->h_dest,h_dest[0],h_dest[1],h_dest[2],h_dest[3],h_dest[4],h_dest[5]);

    // Add IP header
    new_iphdr->version = 4;
    new_iphdr->ihl = 5;
    new_iphdr->tot_len = htons(pkt_size + sizeof(struct iphdr) + sizeof(struct udphdr) + sizeof(struct vxlanhdr));
    new_iphdr->protocol = IPPROTO_UDP;
    new_iphdr->saddr=saddr;
    new_iphdr->daddr=daddr;

    // Add UDP header
    new_udphdr->source=htons(source);
    new_udphdr->dest=htons(dest);
    new_udphdr->len = htons(pkt_size + sizeof(struct udphdr) + sizeof(struct vxlanhdr));

    // Add VXLAN header
    vxlanhdr->flags = htonl(0x08000000); // Set the I flag
    vxlanhdr->vni_reserved2 = htonl(0x123456 << 8); // Example VNI

    // print_ether_hdr(new_ethhdr);

    return 0;
}

int check_match_action_table(struct rte_mbuf * m, uint8_t * pkt, int pkt_size) {
    pthread_rwlock_rdlock(&fsm_lock);
    struct match_action_table_entry * entry;
    int i = 0;
    list_for_each_entry(entry, &match_action_table, list) {
        if (entry->cond_check && entry->cond_check(m,pkt,pkt_size,entry->udp_dest)) {
            entry->callback_fn(m, pkt, pkt_size, entry->h_dest, entry->h_source, entry->saddr, entry->daddr, entry->source, entry->dest, entry->vni);
            break;
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

#define BE_IPV4_ADDR(a, b, c, d)    (((uint32_t)a<<24) + (b<<16) + (c<<8) + d)

int main(int argc, char ** argv) {
	int ret;
    if ((ret = rte_eal_init(argc, argv)) < 0) {
		rte_exit(EXIT_FAILURE, "Cannot init EAL: %s\n", rte_strerror(rte_errno));
    }

    argc -= ret;
	argv += ret;

    nb_cores = rte_lcore_count();

    config_ports();

    pthread_rwlockattr_t attr;
    pthread_rwlockattr_init(&attr);
    pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    if (pthread_rwlock_init(&fsm_lock, &attr) != 0) {
        perror("Failed to initialize rwlock");
    }

    init_list_head(&match_action_table);

    for (int i = 0; i < 10; i++) {
        struct match_action_table_entry * entry = (struct match_action_table_entry *)calloc(1, sizeof(struct match_action_table_entry));
        entry->udp_dest = 1234 + i;
        entry->cond_check = cond_check;

        uint8_t h_source[6] = {0xde,0xed,0xbe,0xef,0xab,0xcd};
        uint8_t h_dest[6] = {0x10,0x70,0xfd,0xc8,0x94,0x75};
        uint32_t saddr = BE_IPV4_ADDR(8,8,8,i);
        uint32_t daddr = BE_IPV4_ADDR(1,2,3,i);
        uint16_t source = 4798;
        uint16_t dest = 1234 + i;

        memcpy(entry->h_source, h_source, 6);
        memcpy(entry->h_dest, h_dest, 6);
        entry->saddr = saddr;
        entry->daddr = daddr;
        entry->source = source;
        entry->dest = dest;

        entry->callback_fn = callback_fn;
        list_add_tail(&entry->list, &match_action_table);        
    }

    rte_eal_mp_remote_launch(launch_one_lcore, NULL, CALL_MAIN);
    rte_eal_mp_wait_lcore();

    /* clean up the EAL */
	rte_eal_cleanup();
	printf("Bye...\n");

    return 0;
}
