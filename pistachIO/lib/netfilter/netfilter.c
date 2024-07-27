#include <assert.h>
#include <stdlib.h>

#include "opt.h"
#include "printk.h"
#include "linux/netfilter.h"

#include <rte_hash.h>
#include <rte_tailq.h>

struct flow_info {
    uint32_t    saddr;
	uint32_t    daddr;
    uint16_t	sport;
    uint16_t    dport;
    uint8_t     proto;
};

struct rte_tailq_entry_head * pre_routing_tbl;
struct rte_tailq_entry_head * local_in_tbl;
struct rte_tailq_entry_head * forward_tbl;
struct rte_tailq_entry_head * local_out_tbl;
struct rte_tailq_entry_head * post_routing_tbl;

static int __nf_register_net_hook(struct net * net, int pf, const struct nf_hook_ops * reg) {
    struct rte_tailq_entry_head * tbl;
    // struct rte_tailq_entry new_entry;
    struct rte_tailq_entry * new_entry = calloc(1, sizeof(struct rte_tailq_entry));
	struct nf_hook_entry * p = calloc(1, sizeof(struct nf_hook_entry));

    switch(reg->hooknum) {
        case NF_INET_PRE_ROUTING:
            tbl = pre_routing_tbl;
            break;
        case NF_INET_LOCAL_IN:
            tbl = local_in_tbl;
            break;
        case NF_INET_FORWARD:
            tbl = forward_tbl;
            break;
        case NF_INET_LOCAL_OUT:
            tbl = local_out_tbl;
            break;
        case NF_INET_POST_ROUTING:
            tbl = post_routing_tbl;
            break;
        default:
            pr_warn("Unkown hooknum!\n");
            return 0;
    }

    p->hook = reg->hook;
    p->priv = reg->priv;
    p->cond = reg->cond;

    new_entry->data = (void *)p;
	TAILQ_INSERT_TAIL(tbl, new_entry, next);
    pr_debug(NF_DEBUG, "NEW entry: %p, hook: %p, priv: %p, tailq entry: %p\n", p, p->hook, p->priv, new_entry);

	return 0;
}

int nf_register_net_hook(struct net * net, const struct nf_hook_ops * reg) {
	int err;
	if (reg->pf == NFPROTO_INET) {
        pr_debug(NF_DEBUG, "registering hook...\n");
		err = __nf_register_net_hook(net, NFPROTO_IPV4, reg);
		if (err < 0) {
			return err;
        }
	}

	return 0;
}

int nftnl_init() {
    struct rte_tailq_elem pre_routing_tailq = {
        .name = "pre_routing_table",
    };
    struct rte_tailq_elem local_in_tailq = {
        .name = "local_in_table",
    };
    struct rte_tailq_elem forward_tailq = {
        .name = "forward_table",
    };
    struct rte_tailq_elem local_out_tailq = {
        .name = "local_out_table",
    };
    struct rte_tailq_elem post_routing_tailq = {
        .name = "post_routing_table",
    };

	pre_routing_tbl = RTE_TAILQ_LOOKUP(pre_routing_tailq.name, rte_tailq_entry_head);
    assert(pre_routing_tbl != NULL);

	local_in_tbl = RTE_TAILQ_LOOKUP(local_in_tailq.name, rte_tailq_entry_head);
    assert(local_in_tbl != NULL);

	forward_tbl = RTE_TAILQ_LOOKUP(forward_tailq.name, rte_tailq_entry_head);
    assert(forward_tbl != NULL);

	local_out_tbl = RTE_TAILQ_LOOKUP(local_out_tailq.name, rte_tailq_entry_head);
    assert(local_out_tbl != NULL);

	post_routing_tbl = RTE_TAILQ_LOOKUP(post_routing_tailq.name, rte_tailq_entry_head);
    assert(post_routing_tbl != NULL);

    return 0;
}