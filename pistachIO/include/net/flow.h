#ifndef _FLOW_H_
#define _FLOW_H_

#include <linux/kernel.h>

#include "kernel/bitops.h"

struct flowi_common {
	__u8	flowic_proto;
};

union flowi_uli {
	struct {
		__be16	dport;
		__be16	sport;
	} ports;

	struct {
		__u8	type;
		__u8	code;
	} icmpt;

	struct {
		__le16	dport;
		__le16	sport;
	} dnports;

	__be32		spi;
	__be32		gre_key;

	struct {
		__u8	type;
	} mht;
};

/* In network byte order */
struct flowi4 {
	struct flowi_common	__fl_common;
#define flowi4_proto		__fl_common.flowic_proto

	/* (saddr,daddr) must be grouped, same order as in IP header */
	__be32			saddr;
	__be32			daddr;

	union flowi_uli		uli;
#define fl4_sport		uli.ports.sport
#define fl4_dport		uli.ports.dport
#define fl4_icmp_type	uli.icmpt.type
#define fl4_icmp_code	uli.icmpt.code
#define fl4_ipsec_spi	uli.spi
#define fl4_mh_type		uli.mht.type
#define fl4_gre_key		uli.gre_key
// } __attribute__((__aligned__(BITS_PER_LONG/8)));
} __attribute__((__aligned__(sizeof(long))));

#endif  /* _FLOW_H_ */