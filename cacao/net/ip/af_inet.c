#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>

#include "opt.h"
#include "printk.h"
#include "net/net.h"
#include "net/netfmt.h"
#include "net/tcp.h"
#include "net/udp.h"
#include "net/flow.h"

int inet_release(struct socket * sock) {
	return 0;
}

int __inet_bind(struct sock * sk, const struct sockaddr * uaddr, int addr_len) {
	struct sockaddr_in *addr = (struct sockaddr_in *)uaddr;
	unsigned short snum;

	snum = ntohs(addr->sin_port);

	sk->sk_rcv_saddr = addr->sin_addr.s_addr;
	/* TODO: Assuming port never collide */
	sk->sk_num = htons(snum);

	sk->sk_daddr = 0;
	sk->sk_dport = 0;

    pr_debug(SOCKET_DEBUG, "%s: sock: %p(src ip: " IP_STRING ", dst ip: " IP_STRING " src port: %u, dst port: %u)\n", 
			__func__, sk, NET_IP_FMT(sk->sk_rcv_saddr), NET_IP_FMT(sk->sk_daddr), ntohs(sk->sk_num), ntohs(sk->sk_dport));

	return 0;
}

int inet_bind(struct socket * sock, const struct sockaddr * uaddr, int addr_len) {
	struct sock * sk = &sock->sk;
    pr_debug(SOCKET_DEBUG, "%s: socket: %p, uaddr: %p, addr_len: %d\n", __func__, sock, uaddr, addr_len);

	if (sk->sk_prot->bind) {
		return sk->sk_prot->bind(sk, uaddr, addr_len);
	}

	return __inet_bind(sk, uaddr, addr_len);
}

int inet_stream_connect(struct socket * sock, struct sockaddr * uaddr, int addr_len, int flags) {
	return 0;
}

int inet_dgram_connect(struct socket * sock, struct sockaddr * uaddr, int addr_len, int flags) {
	return 0;
}

int inet_accept(struct socket * sock, struct socket * newsock, int flags) {
	return 0;
}

int inet_listen(struct socket * sock, int backlog) {
	return 0;
}

int inet_sendmsg(struct socket * sock, struct msghdr * msg, size_t size) {
	return 0;
}

int inet_recvmsg(struct socket * sock, struct msghdr * msg, size_t size, int flags) {
	return 0;
}

const struct proto_ops inet_stream_ops = {
    .family 		= PF_INET,
	.release		= inet_release,
	.bind 			= inet_bind,
	.connect 		= inet_stream_connect,
	.accept			= inet_accept,
    .listen 		= inet_listen,
    .sendmsg 		= inet_sendmsg,
	.recvmsg 		= inet_recvmsg,
};

const struct proto_ops inet_dgram_ops = {
	.family		   = PF_INET,
	.release	   = inet_release,
	.bind		   = inet_bind,
	.connect	   = inet_dgram_connect,
	.sendmsg	   = inet_sendmsg,
	.recvmsg	   = inet_recvmsg,
};

/**
 * Create an INET socket
 * 
 * @param sock allocated socket structure
 * @param protocol socket protocol type
 * @return 0 if succeeded, errno if failed 
 */
static int inet_create(struct socket * sock, int protocol) {
	struct sock * sk = &sock->sk;
	struct proto * prot = NULL;
	int err = -ENOBUFS;

    if (protocol < 0 || protocol >= IPPROTO_MAX) {
        return -EINVAL;
    }

	sock->state = SS_UNCONNECTED;

	switch(protocol) {
		case IPPROTO_IP:	/* Dummy protocol for TCP */
		case IPPROTO_TCP:
			sock->ops = &inet_stream_ops;
			prot = &tcp_prot;
			break;
		case IPPROTO_UDP:
			sock->ops = &inet_dgram_ops;
			prot = &udp_prot;
			break;
		default:
			pr_warn("Unknown protocol(%d)!", protocol);
			break;
	}

	// spin_lock_init(&sk->sk_lock);
	// mutex_init(&sk->sk_lock);

	sk->sk_prot = prot;
    sk->sk_family = PF_INET;
	sk->sk_protocol = protocol;

	if (sk->sk_prot && sk->sk_prot->init) {
		err = sk->sk_prot->init(sk);
		if (err) {
			// sk_common_release(sk);
			pr_warn("failed to init sock!\n");
			goto out;
		}
	}

out:
    return err;
}

static const struct net_proto_family inet_family_ops = {
	.family = PF_INET,
	.create = inet_create,
};

int __init inet_init(void) {
    /* Register INET family protocol */
	sock_register(&inet_family_ops);
    return 0;
}