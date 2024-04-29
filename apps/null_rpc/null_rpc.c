#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <linux/netfilter.h>
#include <net/skbuff.h>

static unsigned int check_cond(struct sk_buff *skb) {
    return NF_MATCH; // Accept the packet
}

// Function to be called by hook
static unsigned int hook_func(void *priv, struct sk_buff *skb, const struct nf_hook_state *state) {
   return NF_ACCEPT;
}

static struct nf_hook_ops nfho = {
    .cond       = check_cond,
    .hook       = hook_func,
    .hooknum    = NF_INET_PRE_ROUTING,
    .pf         = NFPROTO_INET,
};

int null_rpc_init(void) {
    nf_register_net_hook(NULL, &nfho);
    return 0;
}