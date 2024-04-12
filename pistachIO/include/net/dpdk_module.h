#ifndef _DPDK_MODULE_H_
#define _DPDK_MODULE_H_

#include <stdint.h>

#include <rte_common.h>
#include <rte_eal.h>
#include <rte_flow.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_version.h>

#include "init.h"

/* Maximum number of packets to be retrieved via burst */
#define MAX_PKT_BURST   32

extern uint8_t * dpdk_get_rxpkt(int pid, struct rte_mbuf ** pkts, int index, int * pkt_size);
extern uint32_t dpdk_recv_pkts(int pid, struct rte_mbuf ** pkts);
extern void dpdk_recv_done(struct rte_mbuf ** pkts, int len);
extern struct rte_mbuf * dpdk_alloc_txpkt(int pkt_size);
extern int dpdk_insert_txpkt(int pid, struct rte_mbuf * m);
extern uint32_t dpdk_send_pkts(int port_id);

extern struct rte_mempool * pkt_mempool;

extern int __init dpdk_init(int argc, char ** argv);

#endif  /* _DPDK_MODULE_H_ */