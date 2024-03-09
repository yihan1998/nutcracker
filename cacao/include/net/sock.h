#ifndef _SOCK_H_
#define _SOCK_H_

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <sys/socket.h>

#include "list.h"

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
	int (*setsockopt)(struct sock * sk, int level, int optname, char * optval, unsigned int optlen);

    int (*sendmsg)(struct sock * sk, struct msghdr * msg, size_t len);
	int (*recvmsg)(struct sock * sk, struct msghdr * msg, size_t len, int noblock, int flags, int * addr_len);

    int (*bind)(struct sock * sk, const struct sockaddr * addr, int addrlen);
    int (*listen)(struct sock * sk, int backlog);

    int (*hash)(struct sock * sk, void * key);
	void (*unhash)(struct sock * sk);

	unsigned int obj_size;
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
};

struct sock {
    struct sock_common sk_common;
#define sk_daddr    sk_common.skc_daddr
#define sk_rcv_saddr    sk_common.skc_rcv_saddr
#define sk_dport    sk_common.skc_dport
#define sk_num  sk_common.skc_num
#define sk_family   sk_common.skc_family
#define sk_reuse    sk_common.skc_reuse
#define sk_reuseport    sk_common.skc_reuseport
#define sk_prot     sk_common.skc_prot
#define sk_state    sk_common.skc_state
#define sk_flags    sk_common.skc_flags
#define sk_lock    	sk_common.skc_lock

	struct socket * sk_socket;
    unsigned char sk_protocol;

	struct list_head    sk_receive_queue;
	struct list_head    sk_write_queue;

} __attribute__((aligned(64)));

#endif  /* _SOCK_H_ */