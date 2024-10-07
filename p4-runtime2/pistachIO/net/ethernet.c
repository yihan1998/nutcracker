#include "opt.h"
#include "utils/printk.h"
#include "net/dpdk.h"
#include "net/ethernet.h"
#include "net/ip.h"

void printEthernetHeader(const struct ethhdr *eth) {
    printf("Ethernet Header\n");
    printf("   |-Source MAC      : %02x:%02x:%02x:%02x:%02x:%02x\n",
           eth->h_source[0], eth->h_source[1], eth->h_source[2],
           eth->h_source[3], eth->h_source[4], eth->h_source[5]);
    printf("   |-Destination MAC : %02x:%02x:%02x:%02x:%02x:%02x\n",
           eth->h_dest[0], eth->h_dest[1], eth->h_dest[2],
           eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);
    printf("   |-Protocol        : 0x%04x\n", ntohs(eth->h_proto));
}

int ethernet_input(struct rte_mbuf * m, uint8_t * pkt, int pkt_size) {
    struct ethhdr * ethhdr;
    uint16_t proto;
    struct iphdr * iphdr;
    struct sk_buff * skb;
    int ret = NET_RX_DROP;

    ethhdr = (struct ethhdr *)pkt;
    if (memcmp(ethhdr->h_dest, src_addr, ETH_ALEN) != 0) {
		return NET_RX_DROP;
    }

    skb = alloc_skb(m, pkt, pkt_size);
    if (!skb) {
		pr_warn("Failed to allocate new skbuff!\n");
		return NET_RX_DROP;
	}

#if LATENCY_BREAKDOWN
	struct pktgen_tstamp * ts = (struct pktgen_tstamp *)(skb->pkt + sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr));
    ts->network_recv = get_current_time_ns();
#endif

    skb->mac_header = 0;
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
