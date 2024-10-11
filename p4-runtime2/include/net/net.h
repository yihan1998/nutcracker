#ifndef _NET_H_
#define _NET_H_

#include <stdint.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <rte_hash.h>
#include <rte_fbk_hash.h>
#include <rte_jhash.h>
#include <rte_hash_crc.h>

#include "init.h"
#include "percpu.h"
#include "list.h"
#include "kernel/wait.h"
#include "kernel/poll.h"

#ifdef __cplusplus
extern "C" {
#endif

extern struct rte_hash * flow_table;

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
    int	(*ioctl)(struct socket * sock, unsigned int cmd, unsigned long arg);
	int (*sendmsg)(struct socket * sock, struct msghdr * m, size_t total_len);
	int (*recvmsg)(struct socket * sock, struct msghdr * m, size_t total_len, int flags);
	int (*listen)(struct socket * sock, int backlog);
	int (*shutdown)(struct socket * sock, int flags);
	int	(*setsockopt)(struct socket * sock, int level, int optname, char * optval, unsigned int optlen);
	int	(*getsockopt)(struct socket * sock, int level, int optname, char * optval, int * optlen);
	int (*create_flow)(struct socket * sock, int prior, int core);
};

#define DECLARE_SOCKADDR(type, dst, src)	\
	type dst = ({ (type) src; })

struct net_proto_family {
	int family;
	int (*create)(struct socket * sock, int protocol);
};

extern int sock_register(const struct net_proto_family * ops);

struct network_info {
    uint64_t sec_send;
    uint64_t sec_recv;
    uint64_t total_send;
    uint64_t total_recv;
    struct timespec last_log;
};

DECLARE_PER_CPU(struct network_info, net_info);

extern pthread_spinlock_t rx_lock;
extern pthread_spinlock_t tx_lock;

#if LATENCY_BREAKDOWN
struct pktgen_tstamp {
	uint16_t magic;
    uint64_t send_start;
    uint64_t network_recv;
    uint64_t network_udp_input;
    uint64_t server_recv;
    uint64_t server_recvfrom;
    
    uint64_t app_total_time;
    uint64_t app_preprocess_time;
    uint64_t app_predict_time;
    uint64_t app_recommend_time;
    uint64_t app_compress_time;

    uint64_t server_sendto;
    uint64_t network_udp_output;
    uint64_t network_send;
} __attribute__((packed));
#endif

struct locked_list_head {
    pthread_spinlock_t lock;
    struct list_head head;
};

extern const struct rte_memzone * pending_sk_mz;
extern struct locked_list_head * pending_sk;

#define NET_RX_SUCCESS	0	/* keep 'em coming, baby */
#define NET_RX_DROP		1	/* packet dropped */

struct sk_buff;
int run_state_machine(struct sk_buff * skb);

// extern int __init packet_init(void);
extern int __init net_init(void);
extern int net_loop(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* _NET_H_ */