#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <linux/if_ether.h>
#include <linux/udp.h>

#include "backend/flowPipeInternal.h"
#include "net/skbuff.h"
#include "net/dpdk.h"

uint32_t pipe_id = 1;

struct flow_pipe * instantialize_flow_pipe(const char * name) {
    struct flow_pipe * new_pipe = (struct flow_pipe *)calloc(1, sizeof(struct flow_pipe));
    memcpy(new_pipe->name, name, strlen(name));
    new_pipe->id = pipe_id++;
	printf("[%s:%d] New pipe %s created with id: %d: %p\n", __func__, __LINE__, name, new_pipe->id, new_pipe);
    return new_pipe;
}

uint8_t * get_packet_internal(struct sk_buff * skb) {
    return rte_pktmbuf_mtod(skb->mbuf, uint8_t *);
}

uint32_t get_packet_size_internal(struct sk_buff * skb) {
    return skb->mbuf->pkt_len;
}

struct sk_buff* prepend_packet_internal(struct sk_buff* skb, uint32_t prepend_size) {
    uint8_t *pkt = rte_pktmbuf_mtod(skb->mbuf, uint8_t *);
    int orig_size = skb->mbuf->pkt_len;
    if (rte_pktmbuf_headroom(skb->mbuf) < prepend_size) {
        printf("Not enough space!\n");
        return NULL;
    }
    uint8_t *new_pkt = (uint8_t *)rte_pktmbuf_prepend(skb->mbuf, prepend_size);
    if (new_pkt == NULL) {
        printf("Failed to prepend!\n");
        return NULL;
    }
    memmove(new_pkt+prepend_size, pkt, orig_size);
    skb->pkt_len = skb->mbuf->pkt_len;
    return skb;
}

bool fsm_table_lookup_internal(struct flow_match* pipe_match, struct flow_match* match, struct sk_buff* skb) {
    uint8_t * p = get_packet_internal(skb);
    struct ethhdr * ethhdr = (struct ethhdr *)p;
    struct iphdr * iphdr = (struct iphdr *)&ethhdr[1];
    struct udphdr * udphdr = (struct udphdr *)&iphdr[1];

    bool matched = true;
    if ((pipe_match->outer.udp.dest & udphdr->dest) != match->outer.udp.dest) {
        matched = false;
    }

    return matched;
}