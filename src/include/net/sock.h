#ifndef _SOCK_H_
#define _SOCK_H_

#include <stdint.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <stdatomic.h>
#include <pthread.h>

#include "list.h"
#include "kernel/bitops.h"
#include "kernel/wait.h"
#include "net/net.h"
#include "net/ip.h"
#include "net/udp.h"
#include "net/raw.h"

struct sock_common {
    /* Network byte order */
    uint32_t skc_daddr;
    /* Network byte order */
    uint32_t skc_rcv_saddr;
	/* Network byte order */
    uint16_t skc_dport;
	/* Network byte order */
    uint16_t skc_num;
    unsigned short  skc_family;
    unsigned short  skc_reuse;
    unsigned short  skc_reuseport;
    unsigned char   skc_state;
    unsigned long   skc_flags;
	// pthread_spinlock_t skc_lock;
	pthread_rwlock_t skc_lock;
};

struct sock {
    struct sock_common  sk_common;
#define sk_daddr        sk_common.skc_daddr
#define sk_rcv_saddr    sk_common.skc_rcv_saddr
#define sk_dport        sk_common.skc_dport
#define sk_num          sk_common.skc_num
#define sk_family       sk_common.skc_family
#define sk_reuse        sk_common.skc_reuse
#define sk_reuseport    sk_common.skc_reuseport
#define sk_state        sk_common.skc_state
#define sk_flags        sk_common.skc_flags
#define sk_lock    	    sk_common.skc_lock

	struct socket       * sk_socket;
    struct socket_wq    * sk_wq;
    unsigned char       sk_protocol;

	// void	(*sk_state_change)(struct sock * sk);
	// void	(*sk_data_ready)(struct sock * sk);
	// void	(*sk_write_space)(struct sock * sk);

	long	sk_rcvtimeo;
	long	sk_sndtimeo;

	struct list_head	sk_receive_queue;
	struct list_head 	sk_write_queue;

	uint8_t	sk_tx_pending;

    struct list_head sk_list;

	union {
		struct ip_pcb  ip;
		// struct tcp_pcb tcp;
		struct udp_pcb udp;
		struct raw_pcb raw;
	} pcb;
} __attribute__ ((aligned(64)));

/* Sock flags */
enum sock_flags {
	SOCK_DEAD = 0,
	SOCK_DONE,
	SOCK_URGINLINE,
	SOCK_KEEPOPEN,
	SOCK_LINGER,
	SOCK_DESTROY,
	SOCK_BROADCAST,
	SOCK_TIMESTAMP,
	SOCK_ZAPPED,
	SOCK_USE_WRITE_QUEUE, /* whether to call sk->sk_write_space in sock_wfree */
	SOCK_DBG, /* %SO_DEBUG setting */
	SOCK_RCVTSTAMP, /* %SO_TIMESTAMP setting */
	SOCK_RCVTSTAMPNS, /* %SO_TIMESTAMPNS setting */
	SOCK_LOCALROUTE, /* route locally only, %SO_DONTROUTE setting */
	SOCK_QUEUE_SHRUNK, /* write queue has been shrunk recently */
	SOCK_MEMALLOC, /* VM depends on this socket for swapping */
	SOCK_TIMESTAMPING_RX_SOFTWARE,  /* %SOF_TIMESTAMPING_RX_SOFTWARE */
	SOCK_FASYNC, /* fasync() active */
	SOCK_RXQ_OVFL,
	SOCK_ZEROCOPY, /* buffers from userspace */
	SOCK_WIFI_STATUS, /* push wifi status to userspace */
	SOCK_NOFCS, /* Tell NIC not to do the Ethernet FCS.
		     * Will use last 4 bytes of packet sent from
		     * user-space instead.
		     */
	SOCK_FILTER_LOCKED, /* Filter cannot be changed anymore */
	SOCK_SELECT_ERR_QUEUE, /* Wake select on error queue */
	SOCK_RCU_FREE, /* wait rcu grace period in sk_destruct() */
	SOCK_TXTIME,
	SOCK_XDP, /* XDP is attached */
	SOCK_TSTAMP_NEW, /* Indicates 64 bit timestamps always */
};

static inline void sock_set_flag(struct sock * sk, enum sock_flags flag) {
	set_bit(flag, &sk->sk_flags);
}

static inline bool sock_flag(struct sock * sk, enum sock_flags flag) {
	return test_bit(flag, &sk->sk_flags);
}

static inline long sock_rcvtimeo(const struct sock * sk, bool noblock) {
	return noblock ? 0 : sk->sk_rcvtimeo;
}

static inline long sock_sndtimeo(const struct sock * sk, bool noblock) {
	return noblock ? 0 : sk->sk_sndtimeo;
}

// static inline void lock_sock(struct sock * sk) {
// 	pthread_spin_lock(&sk->sk_lock);
// }

// static inline void unlock_sock(struct sock * sk) {
// 	pthread_spin_unlock(&sk->sk_lock);
// }

static inline void rdlock_sock(struct sock * sk) {
	pthread_rwlock_rdlock(&sk->sk_lock);
}

static inline void wrlock_sock(struct sock * sk) {
	pthread_rwlock_wrlock(&sk->sk_lock);
}

static inline void unlock_sock(struct sock * sk) {
	pthread_rwlock_unlock(&sk->sk_lock);
}

/* Tells us if the sock is currently linked in TX list */
static inline bool sock_is_pending(struct sock * sk) {
	return atomic_load(&sk->sk_tx_pending);
}

extern void sock_init_data(struct socket * sock, struct sock * sk);

// extern struct sock * sk_alloc(int family, struct proto * prot);
extern struct sock * sk_alloc(int family);
extern void sk_common_release(struct sock * sk);

/**
 * skwq_has_sleeper - check if there are any waiting processes
 * @wq: struct socket_wq
 *
 * Returns true if socket_wq has waiting processes
 */
static inline bool skwq_has_sleeper(struct socket_wq * wq) {
	return wq && wq_has_sleeper(&wq->wait);
}

static inline void sock_poll_wait(struct file * filp, struct socket * sock, poll_table * p) {
	if (!poll_does_not_wait(p)) {
		poll_wait(filp, &sock->wq.wait, p);
	}
}

extern __poll_t datagram_poll(struct file * file, struct socket * sock, struct poll_table_struct * wait);

int sock_common_getsockopt(struct socket * sock, int level, int optname, char * optval, int * optlen);
int sock_common_recvmsg(struct socket * sock, struct msghdr *msg, size_t size, int flags);
int sock_common_setsockopt(struct socket * sock, int level, int optname, char * optval, unsigned int optlen);

#endif  /* _SOCK_H_ */