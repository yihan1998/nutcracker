#include "ip.h"
#include "udp.h"
#include "tcp.h"

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