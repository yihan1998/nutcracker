#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "printk.h"
#include "net/net.h"
#include "net/sock.h"
#include "syscall.h"

static int unix_release(struct socket * sock) {
    return 0;
}

static int unix_bind(struct socket * sock, const struct sockaddr * addr, int addrlen) {
    return 0;
}

static int unix_connect(struct socket * sock, struct sockaddr * addr, int addrlen, int flags) {
    return 0;
}

static int unix_sendmsg(struct socket * sock, struct msghdr * m, size_t total_len) {
    return 0;
}

static int unix_recvmsg(struct socket * sock, struct msghdr * m, size_t total_len, int flags) {
    return 0;
}

static int unix_shutdown(struct socket * sock, int mode) {
    return 0;
}

/* Protocol operations for AF_UNIX socket */
static const struct proto_ops unix_ops = {
    .family     = PF_UNIX,
	.release    = unix_release,
    .bind       = unix_bind,
	.connect    = unix_connect,
    .sendmsg    = unix_sendmsg,
	.recvmsg    = unix_recvmsg,
	.shutdown   = unix_shutdown,
};

static struct proto unix_proto = {
	.name			= "UNIX",
};

/**
 * Create an UNIX socket
 * 
 * @param sock allocated socket structure
 * @param protocol socket protocol type
 * @return 0 if succeeded, errno if failed
 */
static int unix_create(struct socket * sock, int protocol) {
    int ret;
    struct sock * sk = &sock->sk;

	if (protocol && protocol != PF_UNIX) {
		return -EPROTONOSUPPORT;
    }

	sock->state = SS_UNCONNECTED;
    switch (sock->type) {
        case SOCK_STREAM:
        case SOCK_DGRAM:
            sock->ops = &unix_ops;
            break;
        default:
            return -ESOCKTNOSUPPORT;
	}

    sk->sk_prot = &unix_proto;
    sk->sk_family = PF_UNIX;

    ret = libc_socket(sock->ops->family, sock->type, protocol);
    if (ret < 0) {
        pr_err("Fail to allocate file descriptor from Linux(err: %d)\n", errno);
        return ret;
    }

	return 0;
}

static const struct net_proto_family unix_family_ops = {
	.family = PF_UNIX,
	.create = unix_create,
};

int __init unix_init(void) {
    /* Register UNIX family protocol */
	sock_register(&unix_family_ops);
    return 0;
}