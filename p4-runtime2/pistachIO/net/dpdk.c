#define _GNU_SOURCE
#include <stdio.h>
#include <sched.h>
#include <stdatomic.h>

#include "utils/printk.h"
#include "net/net.h"
#include "net/dpdk.h"
#include "net/ethernet.h"

#include <doca_flow.h>

#define RSS_KEY_LEN 40

struct dpdk_config dpdk_config = {
    .port_config.nb_ports = 2,
    .port_config.nb_queues = 1,
    .port_config.nb_hairpin_q = 4,
	.port_config.enable_mbuf_metadata = 1,
};

struct mbuf_table {
    int len;
    struct rte_mbuf * mtable[MAX_MTABLE_SIZE];
} __rte_cache_aligned;
__thread struct mbuf_table tx_mbufs[RTE_MAX_ETHPORTS];


static doca_error_t
setup_hairpin_queues(uint16_t port_id, uint16_t peer_port_id, uint16_t *reserved_hairpin_q_list, int hairpin_queue_len)
{
	/* Port:
	 *	0. RX queue
	 *	1. RX hairpin queue rte_eth_rx_hairpin_queue_setup
	 *	2. TX hairpin queue rte_eth_tx_hairpin_queue_setup
	 */

	int result = 0, hairpin_q;
	uint16_t nb_tx_rx_desc = 2048;
	uint32_t manual = 1;
	uint32_t tx_exp = 1;
	struct rte_eth_hairpin_conf hairpin_conf = {
	    .peer_count = 1,
	    .manual_bind = !!manual,
	    .tx_explicit = !!tx_exp,
	    .peers[0] = {peer_port_id, 0},
	};

	for (hairpin_q = 0; hairpin_q < hairpin_queue_len; hairpin_q++) {
		// TX
		hairpin_conf.peers[0].queue = reserved_hairpin_q_list[hairpin_q];
		result = rte_eth_tx_hairpin_queue_setup(port_id, reserved_hairpin_q_list[hairpin_q], nb_tx_rx_desc,
						     &hairpin_conf);
		if (result < 0) {
			printf("Failed to setup hairpin queues (%d)", result);
			return DOCA_ERROR_DRIVER;
		}

		// RX
		hairpin_conf.peers[0].queue = reserved_hairpin_q_list[hairpin_q];
		result = rte_eth_rx_hairpin_queue_setup(port_id, reserved_hairpin_q_list[hairpin_q], nb_tx_rx_desc,
						     &hairpin_conf);
		if (result < 0) {
			printf("Failed to setup hairpin queues (%d)", result);
			return DOCA_ERROR_DRIVER;
		}
	}
	return DOCA_SUCCESS;
}

int port_init(struct rte_mempool *mbuf_pool, uint8_t port, struct dpdk_config *config) {
    doca_error_t result;
	int ret = 0;
	int symmetric_hash_key_length = RSS_KEY_LEN;
	const uint16_t nb_hairpin_queues = config->port_config.nb_hairpin_q;
	const uint16_t rx_rings = config->port_config.nb_queues;
	const uint16_t tx_rings = config->port_config.nb_queues;
    printf("NB QUEUE: %d, NB HAIRPIN: %d\n", config->port_config.nb_queues, config->port_config.nb_hairpin_q);
	// const uint16_t rss_support = !!(config->port_config.rss_support &&
	// 			       (config->port_config.nb_queues > 1));
	// bool isolated = !!app_config->port_config.isolated_mode;
	uint16_t q, queue_index;
	uint16_t rss_queue_list[nb_hairpin_queues];
	struct rte_ether_addr addr;
	struct rte_eth_dev_info dev_info;
	// struct rte_flow_error error;
	uint8_t symmetric_hash_key[RSS_KEY_LEN] = {
	    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
	    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
	    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
	};
	const struct rte_eth_conf port_conf_default = {
	    .rx_adv_conf = {
		    .rss_conf = {
			    .rss_key_len = symmetric_hash_key_length,
			    .rss_key = symmetric_hash_key,
			    .rss_hf = (RTE_ETH_RSS_IP | RTE_ETH_RSS_UDP | RTE_ETH_RSS_TCP),
			},
		},
	};
	struct rte_eth_conf port_conf = port_conf_default;

	ret = rte_eth_dev_info_get(port, &dev_info);
	if (ret < 0) {
		printf("Failed getting device (port %u) info, error=%s", port, strerror(-ret));
		return -1;
	}
	// port_conf.rxmode.mq_mode = rss_support ? RTE_ETH_MQ_RX_RSS : RTE_ETH_MQ_RX_NONE;
	port_conf.rxmode.mq_mode = RTE_ETH_MQ_RX_RSS;

	/* Configure the Ethernet device */
	ret = rte_eth_dev_configure(port, rx_rings + nb_hairpin_queues, tx_rings + nb_hairpin_queues, &port_conf);
	if (ret < 0) {
		printf("Failed to configure the ethernet device - (%d)", ret);
		return -1;
	}
	if (port_conf_default.rx_adv_conf.rss_conf.rss_hf != port_conf.rx_adv_conf.rss_conf.rss_hf) {
		printf("Port %u modified RSS hash function based on hardware support, requested:%#" PRIx64
			     " configured:%#" PRIx64 "",
			     port, port_conf_default.rx_adv_conf.rss_conf.rss_hf,
			     port_conf.rx_adv_conf.rss_conf.rss_hf);
	}

	/* Enable RX in promiscuous mode for the Ethernet device */
	ret = rte_eth_promiscuous_enable(port);
	if (ret < 0) {
		printf("Failed to Enable RX in promiscuous mode - (%d)", ret);
		return -1;
	}

	/* Allocate and set up RX queues according to number of cores per Ethernet port */
	for (q = 0; q < rx_rings; q++) {
		ret = rte_eth_rx_queue_setup(port, q, RX_DESC_DEFAULT, rte_eth_dev_socket_id(port), NULL, mbuf_pool);
		if (ret < 0) {
			printf("Failed to set up RX queues - (%d)", ret);
			return -1;
		}
	}

	/* Allocate and set up TX queues according to number of cores per Ethernet port */
	for (q = 0; q < tx_rings; q++) {
		ret = rte_eth_tx_queue_setup(port, q, TX_DESC_DEFAULT, rte_eth_dev_socket_id(port), NULL);
		if (ret < 0) {
			printf("Failed to set up TX queues - (%d)", ret);
			return -1;
		}
	}

	/* Enabled hairpin queue before port start */
	if (nb_hairpin_queues && rte_eth_dev_is_valid_port(port ^ 1)) {
		/* Hairpin to both self and peer */
		assert((nb_hairpin_queues % 2) == 0);
		for (queue_index = 0; queue_index < nb_hairpin_queues / 2; queue_index++)
			rss_queue_list[queue_index] = config->port_config.nb_queues + queue_index * 2;
		result = setup_hairpin_queues(port, port, rss_queue_list, nb_hairpin_queues / 2);
		if (result != DOCA_SUCCESS) {
			printf("Cannot hairpin self port %" PRIu8 ", ret: %s",
				     port, doca_get_error_string(result));
			return result;
		}
		for (queue_index = 0; queue_index < nb_hairpin_queues / 2; queue_index++)
			rss_queue_list[queue_index] = config->port_config.nb_queues + queue_index * 2 + 1;
		result = setup_hairpin_queues(port, port ^ 1, rss_queue_list, nb_hairpin_queues / 2);
		if (result != DOCA_SUCCESS) {
			printf("Cannot hairpin peer port %" PRIu8 ", ret: %s",
				     port ^ 1, doca_get_error_string(result));
			return result;
		}
	} else if (nb_hairpin_queues) {
		/* Hairpin to self or peer */
		for (queue_index = 0; queue_index < nb_hairpin_queues; queue_index++)
			rss_queue_list[queue_index] = config->port_config.nb_queues + queue_index;
		if (rte_eth_dev_is_valid_port(port ^ 1))
			result = setup_hairpin_queues(port, port ^ 1, rss_queue_list, nb_hairpin_queues);
		else
			result = setup_hairpin_queues(port, port, rss_queue_list, nb_hairpin_queues);
		if (result != DOCA_SUCCESS) {
			printf("Cannot hairpin port %" PRIu8 ", ret=%d", port, result);
			return result;
		}
	}

	// /* Set isolated mode (true or false) before port start */
	// ret = rte_flow_isolate(port, isolated, &error);
	// if (ret < 0) {
	// 	printf("Port %u could not be set isolated mode to %s (%s)",
	// 		     port, isolated ? "true" : "false", error.message);
	// 	return -1;
	// }
	// if (isolated)
	// 	printf("Ingress traffic on port %u is in isolated mode",
	// 		      port);

	/* Start the Ethernet port */
	ret = rte_eth_dev_start(port);
	if (ret < 0) {
		printf("Cannot start port %" PRIu8 ", ret=%d", port, ret);
		return -1;
	}

	/* Display the port MAC address */
	rte_eth_macaddr_get(port, &addr);
	printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
		     (unsigned int)port, addr.addr_bytes[0], addr.addr_bytes[1], addr.addr_bytes[2], addr.addr_bytes[3],
		     addr.addr_bytes[4], addr.addr_bytes[5]);

	/*
	 * Check that the port is on the same NUMA node as the polling thread
	 * for best performance.
	 */
	if (rte_eth_dev_socket_id(port) > 0 && rte_eth_dev_socket_id(port) != (int)rte_socket_id()) {
		printf("Port %u is on remote NUMA node to polling thread", port);
		printf("\tPerformance will not be optimal");
	}
	return 0;
}

static doca_error_t bind_hairpin_queues(uint16_t port_id) {
	/* Configure the Rx and Tx hairpin queues for the selected port */
	int result = 0, peer_port, peer_ports_len;
	uint16_t peer_ports[RTE_MAX_ETHPORTS];

	/* bind current Tx to all peer Rx */
	peer_ports_len = rte_eth_hairpin_get_peer_ports(port_id, peer_ports, RTE_MAX_ETHPORTS, 1);
	if (peer_ports_len < 0) {
		pr_err("Failed to get hairpin peer Rx ports of port %d, (%d)\n", port_id, peer_ports_len);
		return DOCA_ERROR_DRIVER;
	}
	for (peer_port = 0; peer_port < peer_ports_len; peer_port++) {
		result = rte_eth_hairpin_bind(port_id, peer_ports[peer_port]);
		if (result < 0) {
			pr_err("Failed to bind hairpin queues (%d)\n", result);
			return DOCA_ERROR_DRIVER;
		} else {
            pr_info("Bind current Tx to all peer Rx => Binding port %d to %d\n", peer_port, port_id);
        }
	}
	/* bind all peer Tx to current Rx */
	peer_ports_len = rte_eth_hairpin_get_peer_ports(port_id, peer_ports, RTE_MAX_ETHPORTS, 0);
	if (peer_ports_len < 0) {
		pr_err("Failed to get hairpin peer Tx ports of port %d, (%d)\n", port_id, peer_ports_len);
		return DOCA_ERROR_DRIVER;
	}

	for (peer_port = 0; peer_port < peer_ports_len; peer_port++) {
		result = rte_eth_hairpin_bind(peer_ports[peer_port], port_id);
		if (result < 0) {
			pr_err("Failed to bind hairpin queues (%d)\n", result);
			return DOCA_ERROR_DRIVER;
		} else {
            pr_info("Bind all peer Tx to current Rx => Binding port %d to %d\n", peer_port, port_id);
        }
	}
	return DOCA_SUCCESS;
}

static doca_error_t
enable_hairpin_queues(uint8_t nb_ports)
{
	uint16_t port_id;
	uint16_t n = 0;
	doca_error_t result;

	for (port_id = 0; port_id < RTE_MAX_ETHPORTS; port_id++) {
		if (!rte_eth_dev_is_valid_port(port_id))
			/* the device ID  might not be contiguous */
			continue;
		result = bind_hairpin_queues(port_id);
		if (result != DOCA_SUCCESS) {
			printf("Hairpin bind failed on port=%u", port_id);
			return result;
		}
		if (++n >= nb_ports)
			break;
	}
	return DOCA_SUCCESS;
}

int dpdk_init(int argc, char ** argv) {
    int portid;
	doca_error_t result;
	int ret = 0;

    rte_eal_init(argc, argv);

    dpdk_config.mbuf_pool = rte_pktmbuf_pool_create("pkt_mempool", N_MBUF, RTE_MEMPOOL_CACHE_MAX_SIZE, 0, DEFAULT_MBUF_SIZE, rte_socket_id());
    assert(dpdk_config.mbuf_pool != NULL);

	ret = rte_lcore_count();
	if (ret < dpdk_config.port_config.nb_queues) {
		dpdk_config.port_config.nb_queues = ret;
	}

	if (dpdk_config.port_config.enable_mbuf_metadata) {
		ret = rte_flow_dynf_metadata_register();
		if (ret < 0) {
			printf("Metadata register failed, ret=%d", ret);
			return DOCA_ERROR_DRIVER;
		}
	}

	RTE_ETH_FOREACH_DEV(portid) {
        port_init(dpdk_config.mbuf_pool, portid, &dpdk_config);
    }

    /* Enable hairpin queues */
	if (dpdk_config.port_config.nb_hairpin_q > 0) {
		result = enable_hairpin_queues(dpdk_config.port_config.nb_ports);
		if (result != DOCA_SUCCESS)
			return -1;
	}

    return 0;
}

uint8_t * dpdk_get_rxpkt(int pid, struct rte_mbuf ** pkts, int index, int * pkt_size) {
    struct rte_mbuf * rx_pkt = pkts[index];
    *pkt_size = rx_pkt->pkt_len;
    return rte_pktmbuf_mtod(rx_pkt, uint8_t *);
}

uint32_t dpdk_recv_pkts(int pid, struct rte_mbuf ** pkts) {
    return rte_eth_rx_burst((uint8_t)pid, 0, pkts, MAX_PKT_BURST);
}

struct rte_mbuf * dpdk_alloc_txpkt(int pkt_size) {
    struct rte_mbuf * tx_pkt = rte_pktmbuf_alloc(dpdk_config.mbuf_pool);
    if (!tx_pkt) return NULL;
    tx_pkt->pkt_len = tx_pkt->data_len = pkt_size;
    tx_pkt->nb_segs = 1;
    tx_pkt->next = NULL;
    return tx_pkt;
}

int dpdk_insert_txpkt(int pid, struct rte_mbuf * m) {
    if (unlikely(tx_mbufs[pid].len == MAX_MTABLE_SIZE)) {
        return -1;
    }

    int next_pkt = tx_mbufs[pid].len;
    tx_mbufs[pid].mtable[next_pkt] = m;
    tx_mbufs[pid].len++;
    return 0;
}

uint32_t dpdk_send_pkts(int pid) {
    int total_pkt, pkt_cnt;
    total_pkt = pkt_cnt = tx_mbufs[pid].len;

    struct rte_mbuf ** pkts = tx_mbufs[pid].mtable;

    if (pkt_cnt > 0) {
        int ret;
        do {
            /* Send packets until there is none in TX queue */
            ret = rte_eth_tx_burst(pid, 0, pkts, pkt_cnt);
            pkts += ret;
            pkt_cnt -= ret;
        } while (pkt_cnt > 0);

        tx_mbufs[pid].len = 0;
    }

    return total_pkt;
}

int dpdk_recv(void) {
    struct timespec curr;
    struct rte_mbuf * pkts[MAX_PKT_BURST];
    int nb_recv = 0, nb_send = 0;
	int portid;

    clock_gettime(CLOCK_REALTIME, &curr);
    if (curr.tv_sec - net_info.last_log.tv_sec >= 1) {
        pr_info("[NET] receive %d packets, send %d packets\n", net_info.sec_recv, net_info.sec_send);
        net_info.sec_recv = net_info.sec_send = 0;
        net_info.last_log = curr;
    }

	RTE_ETH_FOREACH_DEV(portid) {
	    pthread_spin_lock(&rx_lock);
		// nb_recv = rte_eth_rx_burst(portid, 0, pkts, MAX_PKT_BURST);
	    nb_recv = dpdk_recv_pkts(portid, pkts);
    	pthread_spin_unlock(&rx_lock);
		if (nb_recv) {
			net_info.sec_recv += nb_recv;
			// printf("[PORT %d] Receive %d packets\n", portid, nb_recv);
			for (int i = 0; i < nb_recv; i++) {
				struct rte_mbuf * rx_pkt = pkts[i];
                int pkt_size = rx_pkt->pkt_len;
                uint8_t * pkt = rte_pktmbuf_mtod(rx_pkt, uint8_t *);
				// if (rte_flow_dynf_metadata_avail()) {
				// 	printf("Packet %d meta data %x\n", i, *RTE_FLOW_DYNF_METADATA(rx_pkt));
				// }
                ethernet_input(rx_pkt, pkt, pkt_size);
			}
		}
	    pthread_spin_lock(&tx_lock);
		nb_send = rte_eth_tx_burst(portid, 0, pkts, nb_recv);
	    pthread_spin_unlock(&tx_lock);
		if (nb_send) {
			net_info.sec_send += nb_send;
			if (unlikely(nb_send < nb_recv)) {
				do {
					rte_pktmbuf_free(pkts[nb_send]);
				} while (++nb_send < nb_recv);
			}
		}
	}
    return nb_recv;
}
