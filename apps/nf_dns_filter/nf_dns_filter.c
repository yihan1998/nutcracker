#include <stdio.h>
#include <arpa/inet.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <net/skbuff.h>

// Function to be called by hook
static unsigned int hook_func(void *priv, struct sk_buff *skb, const struct nf_hook_state *state) {
    struct ethhdr * ethhdr;
    struct iphdr * iphdr;

    if (!skb) return NF_ACCEPT;

    ethhdr = (struct ethhdr *)skb->ptr;
    iphdr = (struct iphdr *)&ethhdr[1];

    // Log destination IP address of the outgoing packet
    // printf("Outgoing packet to %pI4\n", iphdr->daddr);

    return NF_ACCEPT; // Accept the packet
}

static struct nf_hook_ops nfho = {
    .hook       = hook_func,
    .hooknum    = NF_INET_PRE_ROUTING,
    .pf         = NFPROTO_INET,
};

int nf_dns_filter_init(void) {
    // Register the hook
    nf_register_net_hook(NULL, &nfho);
    return 0;
}
