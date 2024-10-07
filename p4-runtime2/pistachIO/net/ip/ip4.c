#include "opt.h"
#include "utils/printk.h"
#include "linux/netfilter.h"
#include "net/netfmt.h"
#include "net/ethernet.h"
#include "net/raw.h"
#include "net/ip.h"
#include "net/udp.h"
#include "net/tcp.h"
#include "net/netns/netfilter.h"

void printIPHeader(const struct iphdr *ipHeader) {
    struct in_addr srcAddr, dstAddr;
    srcAddr.s_addr = ipHeader->saddr;
    dstAddr.s_addr = ipHeader->daddr;

    printf("\nIP Header\n");
    printf("   |-Source IP        : %s\n", inet_ntoa(srcAddr));
    printf("   |-Destination IP   : %s\n", inet_ntoa(dstAddr));
    printf("   |-Protocol         : %d\n", ipHeader->protocol);
}

// Function to print TCP header
void printTCPHeader(const struct tcphdr *tcpHeader) {
    printf("\nTCP Header\n");
    printf("   |-Source Port      : %u\n", ntohs(tcpHeader->source));
    printf("   |-Destination Port : %u\n", ntohs(tcpHeader->dest));
    printf("   |-Sequence Number  : %u\n", ntohl(tcpHeader->seq));
    printf("   |-Acknowledgment   : %u\n", ntohl(tcpHeader->ack_seq));
    printf("   |-Flags            : ");
    if (tcpHeader->syn) printf("SYN ");
    if (tcpHeader->ack) printf("ACK ");
    if (tcpHeader->psh) printf("PSH ");
    if (tcpHeader->rst) printf("RST ");
    if (tcpHeader->fin) printf("FIN ");
    printf("\n");
}

// Function to print UDP header
void printUDPHeader(const struct udphdr *udpHeader) {
    printf("\nUDP Header\n");
    printf("   |-Source Port      : %u\n", ntohs(udpHeader->source));
    printf("   |-Destination Port : %u\n", ntohs(udpHeader->dest));
    printf("   |-Length           : %u\n", ntohs(udpHeader->len));
}

int ip4_input(struct sk_buff * skb, struct iphdr * iphdr) {
    raw_input_state_t raw_status;
    uint16_t iphdr_hlen;
    // uint16_t iphdr_len;
    uint8_t iphdr_protocol;
    struct udphdr * udphdr;
    struct tcphdr * tcphdr;
    int ret = NET_RX_DROP;

    /* obtain IP header length in number of 32-bit words */
    iphdr_hlen = iphdr->ihl;
    /* calculate IP header length in bytes */
    iphdr_hlen <<= 2;
    /* obtain ip length in bytes */
    // iphdr_len = ntohs(iphdr->tot_len);

    iphdr_protocol = iphdr->protocol;

    IPCB(skb)->saddr = iphdr->saddr;
    IPCB(skb)->daddr = iphdr->daddr;
    IPCB(skb)->proto = iphdr->protocol;

    skb->transport_header = (uint8_t *)&iphdr[1] - skb->pkt;

    if (nf_hook(NF_INET_PRE_ROUTING, skb) == NET_RX_DROP) {
        return NET_RX_DROP;
    }

    raw_status = raw_input(skb);
    if (raw_status != RAW_INPUT_EATEN) {
    	switch (iphdr_protocol) {
            case IPPROTO_UDP:
                udphdr = (struct udphdr *)((uint8_t *)iphdr + iphdr_hlen);
                ret = udp_input(skb, iphdr, udphdr);
                break;
            case IPPROTO_TCP:
                tcphdr = (struct tcphdr *)((uint8_t *)iphdr + iphdr_hlen);
                ret = tcp_input(skb, iphdr, tcphdr);
                break;
            default:
                pr_warn("Unrecognized IP layer protocol!\n");
                break;
        }
    }

    // if (ip_route(skb) == NET_RX_SUCCESS) {
    //     return NET_RX_SUCCESS;
    // }
    return NET_RX_SUCCESS;
#if 0
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
#endif
    return ret;
}