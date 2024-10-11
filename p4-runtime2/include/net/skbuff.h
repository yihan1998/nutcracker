#ifndef _SKBUFF_H_
#define _SKBUFF_H_

#include <stdint.h>

// #include <netinet/ip.h>
// #include <netinet/tcp.h>
// #include <netinet/udp.h>

#include "kernel.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "list.h"

struct msghdr;
struct rte_mbuf;
struct sock;

struct sk_buff {
    struct list_head list;

    struct sock * sk;
    struct rte_mbuf * mbuf;

	char    cb[64];

    uint16_t    transport_header;
	uint16_t    network_header;
	uint16_t    mac_header;

    /* Payload length (excluding headers) */
    int data_len;
    uint16_t    data_start;

    /* Total len of skbuff */
    int pkt_len;
    uint8_t * pkt;

    uint8_t buff[0];
} __attribute__ ((aligned(64)));

static inline unsigned char * skb_payload(const struct sk_buff * skb) {
	return (unsigned char *)(skb->pkt + skb->data_start);
}

static inline unsigned char * skb_transport_header(const struct sk_buff * skb) {
	return (unsigned char *)(skb->pkt + skb->transport_header);
}

static inline unsigned char * skb_network_header(const struct sk_buff * skb) {
	return (unsigned char *)(skb->pkt + skb->network_header);
}

static inline unsigned char * skb_mac_header(const struct sk_buff * skb) {
	return (unsigned char *)(skb->pkt + skb->mac_header);
}

static inline bool skb_queue_empty_lockless(const struct list_head * list) {
	return READ_ONCE(list->next) == (const struct list_head *) list;
}

extern struct sk_buff * alloc_skb(struct rte_mbuf * mbuf, uint8_t * data, unsigned int size);
extern void free_skb(struct sk_buff * skb);

extern struct sk_buff * skb_recv_datagram(struct sock * sk, unsigned int flags, int noblock, int * err);
extern struct sk_buff * skb_try_recv_from_queue(struct sock * sk, struct list_head * queue, unsigned int flags, int * err);
extern int skb_wait_for_more_packets(struct sock * sk, int * err, long * timeo_p);

extern int skb_copy_datagram_msg(struct sk_buff * from, int offset, struct msghdr * msg, int size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* _SKBUFF_H_ */