#ifndef _NET_NAMESPACE_H_
#define _NET_NAMESPACE_H_

#include "list.h"
#include "netns/netfilter.h"

struct net {
	struct list_head	list;		/* list of network namespaces */

	struct netns_nf		nf;
};

extern struct net init_net;

#endif  /* _NET_NAMESPACE_H_ */