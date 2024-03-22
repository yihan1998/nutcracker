#include "opt.h"
#include "printk.h"
#include "net/netfmt.h"
#include "net/ethernet.h"
#include "net/ip.h"
#include "net/udp.h"
#include "net/tcp.h"

int ip4_input(uint8_t * pkt, int pkt_size, struct iphdr * iphdr) {
    uint16_t iphdr_hlen;
    // uint16_t iphdr_len;
    uint8_t iphdr_protocol;
    struct udphdr * udphdr;
    struct tcphdr * tcphdr;

    /* obtain IP header length in number of 32-bit words */
    iphdr_hlen = iphdr->ihl;
    /* calculate IP header length in bytes */
    iphdr_hlen *= 4;
    /* obtain ip length in bytes */
    // iphdr_len = ntohs(iphdr->tot_len);

    iphdr_protocol = iphdr->protocol;

    switch (iphdr_protocol) {
        case IPPROTO_UDP:
            udphdr = (struct udphdr *)((uint8_t *)iphdr + iphdr_hlen);
            udp_input(pkt, pkt_size, iphdr, udphdr);
            break;
        case IPPROTO_TCP:
            tcphdr = (struct tcphdr *)((uint8_t *)iphdr + iphdr_hlen);
            tcp_input(pkt, pkt_size, iphdr, tcphdr);
            break;
        default:
            break;
    }

    return 0;
}

int ip4_output(struct sock * sk, struct sk_buff * skb, uint8_t * pkt, int pkt_len) {
    struct iphdr * iphdr;
    iphdr = (struct iphdr *)(pkt + sizeof(struct ethhdr));

    if (skb->iphdr.saddr == INADDR_ANY) {
        iphdr->saddr = inet_addr("10.0.0.1");
    } else {
        iphdr->saddr = skb->iphdr.saddr;
    }

    iphdr->daddr = skb->iphdr.daddr;
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
