#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>

#include "opt.h"
#include "printk.h"
#include "kernel.h"
#include "fs/file.h"
#include "net/net.h"
#include "net/protocol.h"
#include "net/sock.h"
#include "net/udp.h"

static struct list_head inetsw[SOCK_MAX];

/**
 * Create an INET socket
 * 
 * @param sock allocated socket structure
 * @param protocol socket protocol type
 * @return 0 if succeeded, errno if failed 
 */
static int inet_create(struct socket * sock, int protocol) {
	struct sock * sk;
	struct inet_sock * inet;
	struct inet_protosw * protsw;
	struct proto * prot;
	unsigned char flags;
	int err;

    if (protocol < 0 || protocol >= IPPROTO_MAX) {
        return -EINVAL;
    }

    pr_debug(INET_DEBUG, "protocol: %d\n", protocol);

	sock->state = SS_UNCONNECTED;

	list_for_each_entry(protsw, &inetsw[sock->type], list) {
		err = 0;
		if (protocol == protsw->protocol) {
			if (protocol != IPPROTO_IP) {
				break;
			}
		} else {
			if (IPPROTO_IP == protocol) {
				protocol = protsw->protocol;
				break;
			}
			if (IPPROTO_IP == protsw->protocol) {
				break;
			}
		}
		err = -EPROTONOSUPPORT;
	}

	err = -EPERM;

	sock->ops = protsw->ops;
	prot = protsw->prot;
	flags = protsw->flags;

    pr_debug(INET_DEBUG, "allocating sock, prot: %p\n", prot);

	err = -ENOBUFS;
	/* Allocate a new sk */
	sk = sk_alloc(PF_INET, prot);
	if (!sk) {
		pr_err("Failed to allocate sk!\n");
		return err;
	}

	sock_init_data(sock, sk);

	inet = inet_sk(sk);
	inet->is_icsk = (INET_PROTOSW_ICSK & flags) != 0;

	sk->sk_protocol = protocol;
	if (pthread_spin_init(&sk->sk_lock, 0) != 0) {
		pr_err("Failed to init spinlock!\n");
        return err;
    }

	if (sk->sk_prot->init) {
		err = sk->sk_prot->init(sk);
		if (err) {
			sk_common_release(sk);
			goto out;
		}
	}

out:
    return err;
}

static int inet_release(struct socket * sock) {
	// struct sock * sk = sock->sk;
	// int timeout = 0;
	// if (sk) {
	// 	sk->sk_prot->close(sk, timeout);
	// 	sock->sk = NULL;
	// }
	return 0;
}

static int __inet_bind(struct sock * sk, const struct sockaddr * uaddr, int addr_len) {
	return 0;
}

static int inet_bind(struct socket * sock, const struct sockaddr * uaddr, int addr_len) {
	struct sock * sk = sock->sk;
    pr_debug(SOCKET_DEBUG, "%s: Binding socket %p(sk: %p)...\n", __func__, sock, sk);

	/* If the socket has its own bind function then use it. (RAW) */
	if (sk->sk_prot->bind) {
		return sk->sk_prot->bind(sk, uaddr, addr_len);
	}

	return __inet_bind(sk, uaddr, addr_len);
}

int inet_sendmsg(struct socket * sock, struct msghdr * msg, size_t size) {
	struct sock * sk = sock->sk;
	// if (inet_send_prepare(sk)) {
	// 	return -EAGAIN;
	// }
	return sk->sk_prot->sendmsg(sk, msg, size);
}

int inet_recvmsg(struct socket * sock, struct msghdr * msg, size_t size, int flags) {
	struct sock * sk = sock->sk;
	int addr_len = 0;
	return sk->sk_prot->recvmsg(sk, msg, size, flags & MSG_DONTWAIT, flags & ~MSG_DONTWAIT, &addr_len);
}

const struct proto_ops inet_dgram_ops = {
    .family 	= PF_INET,
	.release 	= inet_release,
	.bind 		= inet_bind,
	// .accept = sock_no_accept,
	// .getname	= inet_getname,
	.poll 		= udp_poll,
    // .listen = sock_no_listen,
	// .shutdown = inet_shutdown,
    .sendmsg 	= inet_sendmsg,
	.recvmsg 	= inet_recvmsg,
	// .create_flow = udp_create_flow,
};

static const struct net_proto_family inet_family_ops = {
	.family = PF_INET,
	.create = inet_create,
};

static struct inet_protosw inetsw_array[] = {
	{
		.type = SOCK_STREAM,
		.protocol = IPPROTO_TCP,
		// .prot = &tcp_prot,
		// .ops =  &inet_stream_ops,
		.flags = INET_PROTOSW_PERMANENT | INET_PROTOSW_ICSK,
	},

	{
		.type = SOCK_DGRAM,
		.protocol = IPPROTO_UDP,
		.prot = &udp_prot,
		.ops =  &inet_dgram_ops,
		.flags = INET_PROTOSW_PERMANENT,
    },
};

#define INETSW_ARRAY_LEN ARRAY_SIZE(inetsw_array)

static void inet_register_protosw(struct inet_protosw * p) {
	struct list_head * lh;
	struct inet_protosw * answer;
	int protocol = p->protocol;
	struct list_head * last_perm;

	if (p->type >= SOCK_MAX) {
		goto out_illegal;
	}

	/* If we are trying to override a permanent protocol, bail. */
	last_perm = &inetsw[p->type];
	list_for_each(lh, &inetsw[p->type]) {
		answer = list_entry(lh, struct inet_protosw, list);
		if ((INET_PROTOSW_PERMANENT & answer->flags) == 0) {
			break;
		}
		if (protocol == answer->protocol) {
			goto out_permanent;
		}
		last_perm = lh;
	}

	list_add_tail(&p->list, last_perm);
out:
	return;

out_permanent:
	pr_err(" Attempt to override permanent protocol %d", protocol);
	goto out;

out_illegal:
	pr_err(" Ignoring attempt to register invalid socket type %d", p->type);
	goto out;
}

int __init inet_init(void) {
    struct inet_protosw * q;
	struct list_head * r;

    /* Register the socket-side information for inet_create. */
	for (r = &inetsw[0]; r < &inetsw[SOCK_MAX]; ++r) {
		init_list_head(r);
	}
 
	for (q = inetsw_array; q < &inetsw_array[INETSW_ARRAY_LEN]; ++q) {
		inet_register_protosw(q);
	}

	sock_register(&inet_family_ops); 
	return 0;
}
