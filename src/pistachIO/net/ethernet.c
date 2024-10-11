#include "opt.h"
#include "printk.h"
#include "net/dpdk_module.h"
#include "net/ethernet.h"
#include "net/ip.h"

// Function to print MAC address
void print_mac_address(uint8_t *mac) {
    for (int i = 0; i < 6; i++) {
        printf("%02x", mac[i]);
        if (i < 5) {
            printf(":");
        }
    }
}

// Function to print Ethernet header
void print_ethhdr(struct ethhdr *eth) {
    printf("Ethernet Header:\n");
    printf("  Destination MAC: ");
    print_mac_address(eth->h_dest);
    printf("\n  Source MAC: ");
    print_mac_address(eth->h_source);
    printf("\n  Protocol: 0x%04x\n", ntohs(eth->h_proto));
}

int ethernet_input(struct rte_mbuf * mbuf, uint8_t * pkt, int pkt_size) {
    struct ethhdr * ethhdr;
    uint16_t proto;
    struct iphdr * iphdr;
    struct sk_buff * skb;
    int ret = NET_RX_DROP;

    skb = alloc_skb(mbuf, pkt, pkt_size);
    if (!skb) {
		pr_warn("Failed to allocate new skbuff!\n");
		return NET_RX_DROP;
	}

#if LATENCY_BREAKDOWN
	struct pktgen_tstamp * ts = (struct pktgen_tstamp *)(skb->pkt + sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr));
    ts->network_recv = get_current_time_ns();
#endif

    skb->mac_header = 0;
    ethhdr = (struct ethhdr *)pkt;
    // print_ethhdr(ethhdr);
    proto = ntohs(ethhdr->h_proto);

    skb->network_header = (uint8_t *)&ethhdr[1] - skb->pkt;

    switch (proto) {
        case ETH_P_IP:
            /* pass to IP layer */
            iphdr = (struct iphdr *)&ethhdr[1];
            ret = ip4_input(skb, iphdr);
            break;
        default:
            pr_warn("Unrecognized Ethernet layer protocol!\n");
            break;
    }

    if (ret == NET_RX_DROP) {
        pr_warn("Dropping packet...\n");
        rte_pktmbuf_free(skb->mbuf);
        free_skb(skb);
    }

    return ret;
}

int ethernet_output(struct sock * sk, struct sk_buff * skb, uint8_t * pkt, int pkt_len) {
#if 0
    uint8_t src_eth_addr[ETH_ALEN] = {0x1C,0x34,0xDA,0x5E,0x0E,0xD8};
    uint8_t dst_eth_addr[ETH_ALEN] = {0x1C,0x34,0xDA,0x5E,0x0E,0xD4};
    struct ethhdr * ethhdr;

    ethhdr = (struct ethhdr *)pkt;
    ETHADDR_COPY(ethhdr->h_source, src_eth_addr);
    ETHADDR_COPY(ethhdr->h_dest, dst_eth_addr);
    ethhdr->h_proto = htons(ETH_P_IP);
#endif
    return 0;
}