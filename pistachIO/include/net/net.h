#ifndef _NET_H_
#define _NET_H_

#include <sys/socket.h>
#include <sys/types.h>

#include <rte_hash.h>
#include <rte_fbk_hash.h>
#include <rte_jhash.h>
#include <rte_hash_crc.h>

#include "init.h"
#include "kernel/wait.h"
#include "kernel/poll.h"

#define NR_PROTO    AF_MAX

#define SOCK_MAX	16

#define SOCK_TYPE_MASK 0xf

typedef enum {
	SS_FREE = 0,			/* not allocated		*/
	SS_UNCONNECTED,			/* unconnected to any socket	*/
	SS_CONNECTING,			/* in process of connecting	*/
	SS_CONNECTED,			/* connected to socket		*/
	SS_DISCONNECTING		/* in process of disconnecting	*/
} socket_state;

struct socket_wq {
	/* Note: wait MUST be first field of socket_wq */
	wait_queue_head_t wait;
	unsigned long flags; /* %SOCKWQ_ASYNC_NOSPACE, etc */
};

struct socket {
    socket_state state;
    int type;
    struct file * file;
    struct sock * sk;
    const struct proto_ops * ops;
	struct socket_wq wq;
};

struct proto_ops {
    int family;
	int (*release)(struct socket * sock);
    int (*bind)(struct socket * sock, const struct sockaddr * addr, int addrlen);
	int (*connect)(struct socket * sock, struct sockaddr * addr, int addrlen, int flags);
	int (*accept)(struct socket * sock, struct socket * newsock, int flags);
	int	(*getname)(struct socket * sock, struct sockaddr * addr, int peer);
	__poll_t (*poll)(struct file * file, struct socket * sock, struct poll_table_struct * wait);
    int (*sendmsg)(struct socket * sock, struct msghdr * m, size_t total_len);
	int (*recvmsg)(struct socket * sock, struct msghdr * m, size_t total_len, int flags);
	int (*listen)(struct socket * sock, int backlog);
	int (*shutdown)(struct socket * sock, int flags);
	int	(*setsockopt)(struct socket * sock, int level, int optname, char * optval, unsigned int optlen);
	int (*create_flow)(struct socket * sock, int prior, int core);
};

#define DECLARE_SOCKADDR(type, dst, src)	\
	type dst = ({ (type) src; })

struct net_proto_family {
	int family;
	int (*create)(struct socket * sock, int protocol);
};

extern int sock_register(const struct net_proto_family * ops);

extern struct rte_hash * udp_table;

struct net_stats {
    uint32_t sec_recv_bytes;
    uint32_t sec_send_bytes;
	uint32_t sec_recv_pkts;
    uint32_t sec_send_pkts;
};

extern struct net_stats stats;

extern pthread_spinlock_t rx_lock;
extern pthread_spinlock_t tx_lock;

#define NET_RX_SUCCESS	0	/* keep 'em coming, baby */
#define NET_RX_DROP		1	/* packet dropped */

extern int __init inet_init(void);
extern int __init net_init(void);

#endif  /* _NET_H_ */