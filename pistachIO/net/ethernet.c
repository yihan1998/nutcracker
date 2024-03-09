#include "ethernet.h"
#include "ip.h"

int ethernet_input(uint8_t * pkt, int pkt_size) {
    struct ethhdr * ethhdr;
    uint16_t proto;
    struct iphdr * iphdr;

    ethhdr = (struct ethhdr *)pkt;
    proto = htons(ethhdr->h_proto);

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