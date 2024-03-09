#ifndef _NET_H_
#define _NET_H_

#include "init.h"
#include "sock.h"

#include <rte_hash.h>
#include <rte_fbk_hash.h>
#include <rte_jhash.h>
#include <rte_hash_crc.h>

#define NR_PROTO    AF_MAX

#define SOCK_TYPE_MASK 0xf

typedef enum {
	SS_FREE = 0,			/* not allocated		*/
	SS_UNCONNECTED,			/* unconnected to any socket	*/
	SS_CONNECTING,			/* in process of connecting	*/
	SS_CONNECTED,			/* connected to socket		*/
	SS_DISCONNECTING		/* in process of disconnecting	*/
} socket_state;

/**
 *  struct socket - general BSD socket
 *  @state: socket state (%SS_CONNECTED, etc)
 *  @type: socket type (%SOCK_STREAM, etc)
 *  @flags: socket flags (%SOCK_NOSPACE, etc)
 *  @ops: protocol specific socket operations
 *  @file: File back pointer for gc
 *  @sk: internal networking protocol agnostic socket representation
 *  @wq: wait queue for several uses
 */
struct socket {
	socket_state    state;
	short           type;
	unsigned long   flags;
	const struct proto_ops  * ops;
    struct file 	* file;
	struct sock 	sk;
};

struct proto_ops {
    int family;
	int (*release)(struct socket * sock);
    int (*bind)(struct socket * sock, const struct sockaddr * addr, int addrlen);
	int (*connect)(struct socket * sock, struct sockaddr * addr, int addrlen, int flags);
	int (*accept)(struct socket * sock, struct socket * newsock, int flags);
	int	(*getname)(struct socket * sock, struct sockaddr * addr, int peer);
	int (*sendmsg)(struct socket * sock, struct msghdr * m, size_t total_len);
	int (*recvmsg)(struct socket * sock, struct msghdr * m, size_t total_len, int flags);
	int (*listen)(struct socket * sock, int backlog);
	int (*shutdown)(struct socket * sock, int flags);
	int	(*setsockopt)(struct socket * sock, int level, int optname, char * optval, unsigned int optlen);
	int (*create_flow)(struct socket * sock, int prior, int core);
};

struct net_proto_family {
	int family;
	int (*create)(struct socket * sock, int protocol);
};

extern int sock_register(const struct net_proto_family * ops);

extern struct rte_hash * udp_table;

extern int __init unix_init(void);
extern int __init inet_init(void);
extern int __init net_init(void);

#endif  /* _NET_H_ */