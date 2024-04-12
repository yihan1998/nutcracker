#ifndef _ETHERNET_H_
#define _ETHERNET_H_

#include <stdint.h>
#include <linux/if_ether.h>

#include "net/skbuff.h"
#include "net/sock.h"

#define ETHADDR_COPY(dst, src)  memcpy(dst, src, ETH_ALEN)

extern int ethernet_input(struct rte_mbuf * m, uint8_t * pkt, int pkt_size);
extern int ethernet_output(struct sock * sk, struct sk_buff * skb, uint8_t * pkt, int pkt_len);

#endif  /* _ETHERNET_H_ */