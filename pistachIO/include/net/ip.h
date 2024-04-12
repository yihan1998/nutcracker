#ifndef _IP_H_
#define _IP_H_

#include <stdint.h>
#include <arpa/inet.h>
#include <netinet/ip.h>

#include "net/skbuff.h"
#include "net/sock.h"

#define	IPTOS_ECN_NOT_ECT	0x00

#define DEFAULT_TTL 64  /* Default TTL value */

#define IP_DF   0x4000

struct inet_skb_parm {
	uint32_t    saddr;
	uint32_t    daddr;
};

#define IPCB(skb) ((struct inet_skb_parm *)((skb)->cb))

extern int ip4_input(struct sk_buff * skb, struct iphdr * iphdr);
extern int ip4_output(struct sock * sk, struct sk_buff * skb, uint32_t saddr, uint32_t daddr, uint8_t * pkt, int pkt_len);

#endif  /* _IP_H_ */