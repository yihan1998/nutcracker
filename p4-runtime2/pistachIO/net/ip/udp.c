#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>

#include "opt.h"
#include "utils/printk.h"
#include "kernel.h"
#include "net/dpdk.h"
#include "net/netfmt.h"
#include "net/net.h"
#include "net/skbuff.h"
#include "net/flow.h"
#include "net/sock.h"
#include "net/ethernet.h"
#include "net/udp.h"

static struct sock * udp_lookup_sock(uint32_t saddr, uint32_t sport, uint32_t daddr, uint32_t dport) {
    hash_sig_t hash_value;
	struct sock * try_sk = NULL;
	struct flowi4 key = {0};
	key.saddr       = daddr;
    key.daddr       = 0;
    key.fl4_sport   = dport;
    key.fl4_dport   = 0;
	key.flowi4_proto    = IPPROTO_UDP;

    hash_value = rte_jhash(&key, sizeof(struct flowi4), 0);
    rte_hash_lookup_with_hash_data(flow_table, (const void *)&key, hash_value, (void **)&try_sk);

	if (try_sk) {
		return try_sk;
	}

	return NULL;
}

int udp_input(struct sk_buff * skb, struct iphdr * iphdr, struct udphdr * udphdr) {
    struct sock * sk = NULL;
    uint16_t ulen, len;
	// uint8_t * data;

    sk = udp_lookup_sock(iphdr->saddr, udphdr->source, iphdr->daddr, udphdr->dest);
	if (!sk) {
		sk = udp_lookup_sock(iphdr->saddr, udphdr->source, INADDR_ANY, udphdr->dest);
		if (!sk) {
            pr_warn("Socket not found!\n");
			return NET_RX_DROP;
		}
	}

#if LATENCY_BREAKDOWN
	struct pktgen_tstamp * ts = (struct pktgen_tstamp *)(skb->pkt + sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr));
    ts->network_udp_input = get_current_time_ns();
#endif

	skb->sk = sk;

    ulen = ntohs(udphdr->len);
	len = ulen - sizeof(struct udphdr);
	// data = (uint8_t *)udphdr + sizeof(struct udphdr);

	skb->data_len = len;
    skb->data_start = skb->transport_header + sizeof(struct udphdr);

	memcpy(skb->buff, skb->pkt, skb->pkt_len);
	skb->pkt = skb->buff;

    pr_debug(UDP_DEBUG, "UDP input -> sk %p saddr: " IP_STRING ", sport: %u, daddr: " IP_STRING ", dport: %u, data len: %u\n",
                skb, NET_IP_FMT(ip_hdr(skb)->saddr), ntohs(udp_hdr(skb)->source), 
                NET_IP_FMT(ip_hdr(skb)->daddr), ntohs(udp_hdr(skb)->dest), skb->data_len);

	rte_pktmbuf_free(skb->mbuf);
	skb->mbuf = NULL;

    // lock_sock(sk);
    wrlock_sock(sk);
	list_add_tail(&skb->list, &sk->sk_receive_queue);
	unlock_sock(sk);

	return NET_RX_SUCCESS;
}

int udp_output(struct sock * sk) {
    return 0;
}