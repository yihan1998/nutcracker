#ifndef _DPDK_MODULE_H_
#define _DPDK_MODULE_H_

#include <stdint.h>

#include "init.h"
#include "ipc.h"

extern uint8_t * dpdk_get_rxpkt(int port_id, int index, int * pkt_size);
extern uint32_t dpdk_recv_pkts(int port_id);
extern uint8_t * dpdk_get_txpkt(int port_id, int pkt_size);
extern uint32_t dpdk_send_pkts(int port_id);

extern int __init dpdk_init(int argc, char ** argv);

#endif  /* _DPDK_MODULE_H_ */