#ifndef _NETFILTER_H_
#define _NETFILTER_H_

#include <stdint.h>
#include <sys/socket.h>

#include "linux/netfilter_def.h"

/* Responses from hook functions. */
#define NF_DROP     0
#define NF_ACCEPT   1
#define NF_STOLEN   2
#define NF_QUEUE    3
#define NF_REPEAT   4
#define NF_STOP     5	/* Deprecated, for userspace nf_queue compatibility. */
#define NF_MAX_VERDICT  NF_STOP

struct net_device;
struct sk_buff;
struct nf_hook_ops;
struct sock;
struct net;

struct nf_hook_state {
	unsigned int hook;
	uint8_t pf;
	struct net_device *in;
	struct net_device *out;
	struct sock *sk;
	struct net *net;
	int (*okfn)(struct net *, struct sock *, struct sk_buff *);
};

typedef unsigned int nf_condfn(struct sk_buff *skb);
typedef unsigned int nf_hookfn(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);

struct nf_hook_ops {
	/* User fills in from here down. */
	nf_hookfn   *hook;
	struct net_device   *dev;
	void        *priv;
	nf_condfn	*cond;
	uint8_t     pf;
	unsigned int    hooknum;
	int         priority;
};

struct nf_hook_entry {
	nf_hookfn   *hook;
	nf_condfn	*cond;
	void        *priv;
};

struct nf_hook_entries {
	uint16_t num_hook_entries;
	struct nf_hook_entry hooks[];
};

struct nfcb_task_struct {
	struct nf_hook_entry entry;
	struct sk_buff * skb;
};

int nf_register_net_hook(struct net *net, const struct nf_hook_ops *ops);
void nf_unregister_net_hook(struct net *net, const struct nf_hook_ops *ops);

#endif  /* _NETFILTER_H_ */