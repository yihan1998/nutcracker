#include <errno.h>

#include "err.h"
#include "kernel.h"
#include "opt.h"
#include "printk.h"
#include "linux/netfilter.h"
#include "net/net_namespace.h"

int nf_hook(unsigned int hook, struct sk_buff * skb) {
    struct nf_hook_entry * p;
	struct rte_tailq_entry * entry;
    struct rte_tailq_entry_head * tbl = NULL;

    switch(hook) {
        case NF_INET_PRE_ROUTING:
            tbl = pre_routing_table;
            break;
        case NF_INET_LOCAL_IN:
            tbl = local_in_table;
            break;
        case NF_INET_FORWARD:
            tbl = forward_table;
            break;
        case NF_INET_LOCAL_OUT:
            tbl = local_out_table;
            break;
        case NF_INET_POST_ROUTING:
            tbl = post_routing_table;
            break;
        default:
            pr_warn("Unkown hooknum!\n");
            return 0;
    }

	TAILQ_FOREACH(entry, tbl, next) {
        p = (struct nf_hook_entry *)entry->data;
        if (p->hook) {
            p->hook(p->priv, skb, NULL);
        } else {
            pr_debug(NF_DEBUG, "Entry: %p, hook: %p, priv: %p\n", p, p->hook, p->priv)
        }
    }

    return 0;
}