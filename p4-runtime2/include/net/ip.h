#ifndef _IP_H_
#define _IP_H_

#include <stdint.h>
#include <arpa/inet.h>
#include <netinet/ip.h>

#include "net/skbuff.h"
#include "net/ip_addr.h"

#define	IPTOS_ECN_NOT_ECT	0x00

#define IP_DEFAULT_TTL	64  /* Default TTL value */

#define IP_DF   0x4000

struct inet_skb_parm {
	uint32_t    saddr;
	uint32_t    daddr;
	uint8_t 	proto;
};

#define IPCB(skb) ((struct inet_skb_parm *)((skb)->cb))

/** This is the common part of all PCB types. It needs to be at the
   beginning of a PCB type definition. It is located here so that
   changes to this common part are made in one location instead of
   having to change all PCB structs. */
#define IP_PCB					\
	/* Network byte order */	\
	uint32_t local_ip;			\
	uint32_t remote_ip;			\
	/* Socket options */		\
	uint8_t so_options;			\
	/* Type Of Service */		\
	uint8_t tos;				\
	/* Time To Live */			\
	uint8_t ttl

struct ip_pcb {
	/* Common members of all PCB types */
	IP_PCB;
};

static inline struct iphdr * ip_hdr(const struct sk_buff * skb) {
	return (struct iphdr *)skb_network_header(skb);
}

extern int ip4_input(struct sk_buff * skb, struct iphdr * iphdr);
extern int ip4_output(struct sock * sk, struct sk_buff * skb, uint32_t saddr, uint32_t daddr, uint8_t * pkt, int pkt_len);

#endif  /* _IP_H_ */