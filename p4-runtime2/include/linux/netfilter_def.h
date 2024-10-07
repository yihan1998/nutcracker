#ifndef _NETFILTER_DEF_H_
#define _NETFILTER_DEF_H_

enum nf_inet_hooks {
	NF_INET_PRE_ROUTING = 0,
	NF_INET_LOCAL_IN,
	NF_INET_FORWARD,
	NF_INET_LOCAL_OUT,
	NF_INET_POST_ROUTING,
	NF_INET_NUMHOOKS
};

enum nf_dev_hooks {
	NF_NETDEV_INGRESS = 0,
	NF_NETDEV_NUMHOOKS
};

enum {
	NFPROTO_UNSPEC =  0,
	NFPROTO_INET   =  1,
	NFPROTO_IPV4   =  2,
	NFPROTO_ARP    =  3,
	NFPROTO_NETDEV =  5,
	NFPROTO_BRIDGE =  7,
	NFPROTO_IPV6   = 10,
	NFPROTO_DECNET = 12,
	NFPROTO_NUMPROTO,
};

#define NF_MISS		0
#define NF_MATCH	1

/* Responses from hook functions. */
#define NF_DROP     0
#define NF_ACCEPT   1
#define NF_STOLEN   2
#define NF_QUEUE    3
#define NF_REPEAT   4
#define NF_STOP     5	/* Deprecated, for userspace nf_queue compatibility. */
#define NF_MAX_VERDICT  NF_STOP

#endif  /* _NETFILTER_DEF_H_ */