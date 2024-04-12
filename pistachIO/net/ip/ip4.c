#include "opt.h"
#include "printk.h"
#include "linux/netfilter.h"
#include "net/netfmt.h"
#include "net/ethernet.h"
#include "net/ip.h"
#include "net/udp.h"
#include "net/tcp.h"
#include "net/netns/netfilter.h"

static int ip_route(struct sk_buff * skb) {
    rte_ring_enqueue(fwd_cq, skb);
    return NET_RX_SUCCESS;
}

int ip4_input(struct sk_buff * skb, struct iphdr * iphdr) {
    uint16_t iphdr_hlen;
    // uint16_t iphdr_len;
    uint8_t iphdr_protocol;
    struct udphdr * udphdr;
    struct tcphdr * tcphdr;
    int ret = NET_RX_SUCCESS;

    /* obtain IP header length in number of 32-bit words */
    iphdr_hlen = iphdr->ihl;
    /* calculate IP header length in bytes */
    iphdr_hlen *= 4;
    /* obtain ip length in bytes */
    // iphdr_len = ntohs(iphdr->tot_len);

    iphdr_protocol = iphdr->protocol;

    IPCB(skb)->saddr = iphdr->saddr;
    IPCB(skb)->daddr = iphdr->daddr;

    // nf_hook(NF_INET_PRE_ROUTING, skb);

    if (ip_route(skb) == NET_RX_SUCCESS) {
        return NET_RX_SUCCESS;
    }

    switch (iphdr_protocol) {
        case IPPROTO_UDP:
            udphdr = (struct udphdr *)((uint8_t *)iphdr + iphdr_hlen);
            UDP_SKB_CB(skb)->source = udphdr->source;
            UDP_SKB_CB(skb)->dest = udphdr->dest;
            /* Check if it should be forwarded or processed */
            // nf_hook(NF_INET_LOCAL_IN, skb);
            ret = udp_input(skb, iphdr, udphdr);
            break;
        case IPPROTO_TCP:
            tcphdr = (struct tcphdr *)((uint8_t *)iphdr + iphdr_hlen);
            /* Check if it should be forwarded or processed */
            // nf_hook(NF_INET_LOCAL_IN, skb);
            ret = tcp_input(skb, iphdr, tcphdr);
            break;
        default:
            break;
    }

    return ret;
}

int ip4_output(struct sock * sk, struct sk_buff * skb, uint32_t saddr, uint32_t daddr, uint8_t * pkt, int pkt_len) {
    struct iphdr * iphdr;
    iphdr = (struct iphdr *)(pkt + sizeof(struct ethhdr));

    iphdr->saddr = saddr;
    iphdr->daddr = daddr;
    iphdr->check = 0;
    iphdr->protocol = sk->sk_protocol;
    iphdr->ttl = DEFAULT_TTL;
    iphdr->frag_off = htons(IP_DF);
    iphdr->id = 0;
    iphdr->tot_len = htons(pkt_len - sizeof(struct ethhdr));
	iphdr->tos = IPTOS_ECN_NOT_ECT;
    iphdr->version = 4;
	iphdr->ihl = 5;

    return ethernet_output(sk, skb, pkt, pkt_len);
}
