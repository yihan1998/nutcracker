#ifndef _DPDK_H_
#define _DPDK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <rte_common.h>
#include <rte_eal.h>
#include <rte_flow.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_version.h>

#include "init.h"

#define JUMBO_FRAME_LEN (4096 + RTE_ETHER_CRC_LEN + RTE_ETHER_HDR_LEN)

#define JUMBO_ETHER_MTU \
    (JUMBO_FRAME_LEN - RTE_ETHER_HDR_LEN - RTE_ETHER_CRC_LEN) /**< Ethernet MTU. */

#define N_MBUF              8192
#define DEFAULT_MBUF_SIZE	(JUMBO_FRAME_LEN + RTE_PKTMBUF_HEADROOM) /* See: http://dpdk.org/dev/patchwork/patch/4479/ */

#define RX_DESC_DEFAULT    1024
#define TX_DESC_DEFAULT    1024

#define MAX_MTABLE_SIZE		64
#define MAX_PKT_BURST       32

int __init dpdk_init(int argc, char ** argv);
int dpdk_recv();

struct dpdk_port_config {
	int nb_ports;				/* Set on init to 0 for don't care, required ports otherwise */
	int nb_queues;				/* Set on init to 0 for don't care, required minimum cores otherwise */
	int nb_hairpin_q;			/* Set on init to 0 to disable, hairpin queues otherwise */
	uint16_t enable_mbuf_metadata;
};

/* DPDK configuration */
struct dpdk_config {
	struct dpdk_port_config port_config;
	struct rte_mempool *mbuf_pool;
};

extern pthread_rwlock_t fsm_lock;

extern struct dpdk_config dpdk_config;

extern uint8_t * dpdk_get_rxpkt(int pid, struct rte_mbuf ** pkts, int index, int * pkt_size);
extern uint32_t dpdk_recv_pkts(int pid, struct rte_mbuf ** pkts);
extern struct rte_mbuf * dpdk_alloc_txpkt(int pkt_size);
extern int dpdk_insert_txpkt(int pid, struct rte_mbuf * m);
extern uint32_t dpdk_send_pkts(int port_id);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* _DPDK_H_ */