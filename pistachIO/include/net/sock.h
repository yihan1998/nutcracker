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

struct sock;

struct proto {
	char name[32];
	struct list_head node;
    void (*close)(struct sock * sk, long timeout);
    int (*connect)(struct sock * sk, struct sockaddr * addr, int addrlen);
	int (*disconnect)(struct sock * sk, int flags);

    struct sock * (*accept)(struct sock * sk, int flags, int * err);

    int (*init)(struct sock * sk);
    void (*destroy)(struct sock * sk);
    void (*shutdown)(struct sock * sk, int how);
	int	(*setsockopt)(struct sock * sk, int level, int optname, char * optval, unsigned int optlen);

    int (*sendmsg)(struct sock * sk, struct msghdr * msg, size_t len);
	int (*recvmsg)(struct sock * sk, struct msghdr * msg, size_t len, int noblock, int flags, int * addr_len);

    int (*bind)(struct sock * sk, const struct sockaddr * uaddr, int addr_len);

    int (*hash)(struct sock * sk, void * key);
	void (*unhash)(struct sock * sk);

	int (*get_port)(struct sock * sk, unsigned short snum);

	unsigned int obj_size;

	struct timewait_sock_ops * twsk_prot;
};

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
    struct proto    * skc_prot;
    unsigned char   skc_state;
    unsigned long   skc_flags;
	pthread_spinlock_t skc_lock;
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
#define sk_prot         sk_common.skc_prot
#define sk_state        sk_common.skc_state
#define sk_flags        sk_common.skc_flags
#define sk_lock    	    sk_common.skc_lock

	struct socket       * sk_socket;
    struct socket_wq    * sk_wq;
    unsigned char       sk_protocol;

	void	(*sk_state_change)(struct sock * sk);
	void	(*sk_data_ready)(struct sock * sk);
	void	(*sk_write_space)(struct sock * sk);

	long	sk_rcvtimeo;
	long	sk_sndtimeo;

	uint8_t	sk_tx_pending;
};

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

static inline void lock_sock(struct sock * sk) {
	pthread_spin_lock(&sk->sk_lock);
}

static inline void unlock_sock(struct sock * sk) {
	pthread_spin_unlock(&sk->sk_lock);
}

/* Tells us if the sock is currently linked in TX list */
static inline bool sock_is_pending(struct sock * sk) {
	return atomic_load(&sk->sk_tx_pending);
}

extern void sock_init_data(struct socket * sock, struct sock * sk);

extern struct sock * sk_alloc(int family, struct proto * prot);
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

#endif  /* _SOCK_H_ */