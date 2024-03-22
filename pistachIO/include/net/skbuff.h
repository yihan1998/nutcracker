#ifndef _SKBUFF_H_
#define _SKBUFF_H_

#include "list.h"

#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <rte_mempool.h>

struct sock;

struct sk_buff {
    struct list_head list;

    struct sock * sk;

    struct iphdr iphdr;

    union {
        struct tcphdr tcphdr;
        struct udphdr udphdr;
    };

    int data_len;
	uint8_t data[0];
};

extern struct sk_buff * alloc_skb(unsigned int size);
extern void free_skb(struct sk_buff * skb);

extern struct sk_buff * __skb_try_recv_from_queue(struct sock * sk, unsigned int flags, int * err);
extern int __skb_wait_for_more_packets(struct sock *sk, int *err, long *timeo_p);

extern int skb_copy_datagram_msg(struct sk_buff * from, struct msghdr * msg, int size);

extern struct rte_mempool * skb_mp;

#endif  /* _SKBUFF_H_ */