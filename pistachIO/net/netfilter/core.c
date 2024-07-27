#include <errno.h>

#include "err.h"
#include "kernel.h"
#include "opt.h"
#include "printk.h"
#include "fs/fs.h"
#include "linux/netfilter.h"
#include "kernel/sched.h"
#include "net/dpdk_module.h"
#include "net/net_namespace.h"
#include "net/net.h"

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
        if (p->cond) {
            if (p->cond(skb) == NF_MATCH) {
#ifdef TEST_INLINE
                if (p->hook(p->priv, skb, NULL) == NF_ACCEPT) {
                    while (rte_ring_enqueue(fwd_queue, skb) < 0) {
                    // while (rte_ring_enqueue(nf_cq, skb) < 0) {
                        printf("Failed to enqueue into nf_cq\n");
                    }
                }
#else
                struct nfcb_task_struct * new_entry = NULL;
                do {
                    rte_mempool_get(nftask_mp, (void **)&new_entry);
                    if (new_entry) {
                        new_entry->entry = *p;
                        new_entry->skb = skb;
                        while (rte_ring_enqueue(nf_rq, new_entry) < 0) {
                            printf("Failed to enqueue into nf_rq\n");
                        }
                    }
                } while (!new_entry);
#endif
            }
        } else {
            pr_debug(NF_DEBUG, "Entry: %p, hook: %p, priv: %p\n", p, p->hook, p->priv);
        }
    }

    return NET_RX_SUCCESS;
}