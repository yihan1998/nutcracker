#ifndef _NETNS_NETFILTER_H_
#define _NETNS_NETFILTER_H_

#include "net/flow.h"
#include "net/skbuff.h"
#include "linux/netfilter_def.h"

#include <rte_tailq.h>

struct netns_nf {
	struct nf_hook_entries * hooks_ipv4[NF_INET_NUMHOOKS];
};

extern struct rte_tailq_entry_head * pre_routing_table;
extern struct rte_tailq_entry_head * local_in_table;
extern struct rte_tailq_entry_head * forward_table;
extern struct rte_tailq_entry_head * local_out_table;
extern struct rte_tailq_entry_head * post_routing_table;

extern int nf_hook(unsigned int hook, struct sk_buff * skb);

#endif  /* _NETNS_NETFILTER_H_ */