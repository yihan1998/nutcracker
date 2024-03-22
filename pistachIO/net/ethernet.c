#include "net/ethernet.h"
#include "net/ip.h"

int ethernet_input(uint8_t * pkt, int pkt_size) {
    struct ethhdr * ethhdr;
    uint16_t proto;
    struct iphdr * iphdr;

    ethhdr = (struct ethhdr *)pkt;
    proto = ntohs(ethhdr->h_proto);

    switch (proto) {
        case ETH_P_IP:
            /* pass to IP layer */
            iphdr = (struct iphdr *)&ethhdr[1];
            ip4_input(pkt, pkt_size, iphdr);
            break;
        default:
            break;
    }

    return 0;
}

int ethernet_output(struct sock * sk, struct sk_buff * skb, uint8_t * pkt, int pkt_len) {
    uint8_t src_eth_addr[ETH_ALEN] = {0x1C,0x34,0xDA,0x5E,0x0E,0xD8};
    uint8_t dst_eth_addr[ETH_ALEN] = {0x1C,0x34,0xDA,0x5E,0x0E,0xD4};
    struct ethhdr * ethhdr;

    ethhdr = (struct ethhdr *)pkt;
    ETHADDR_COPY(ethhdr->h_source, src_eth_addr);
    ETHADDR_COPY(ethhdr->h_dest, dst_eth_addr);
    ethhdr->h_proto = htons(ETH_P_IP);

    return 0;
}