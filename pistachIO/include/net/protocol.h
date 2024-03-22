#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#define MAX_INET_PROTOS		256

/* This is used to register socket interfaces for IP protocols.  */
struct inet_protosw {
	struct list_head list;
    /* These two fields form the lookup key.  */
	unsigned short type;	   /* This is the 2nd argument to socket(2). */
	unsigned short protocol; /* This is the L4 protocol number.  */

	struct proto * prot;
	const struct proto_ops * ops;

	unsigned char	flags;      /* See INET_PROTOSW_* below.  */
};

#define INET_PROTOSW_REUSE	0x01	     /* Are ports automatically reusable? */
#define INET_PROTOSW_PERMANENT	0x02  /* Permanent protocols are unremovable. */
#define INET_PROTOSW_ICSK	0x04  /* Is this an inet_connection_sock? */

#endif	/* _PROTOCOL_H_ */