#define _GNU_SOURCE
#include <stdlib.h>
#include <sys/epoll.h>
#include <sched.h>

#include "opt.h"
#include "printk.h"
#include "kernel/wait.h"
#include "net/net.h"
#include "net/sock.h"
#include "net/tcp_states.h"

static void sock_def_readable(struct sock * sk) {
	struct socket_wq * wq;
    wq = sk->sk_wq;
    if (skwq_has_sleeper(wq)) {
        pr_debug(SOCK_DEBUG, "%s: waking up sock %p(wq: %p)\n", __func__, sk, wq);
        wake_up_interruptible_sync_poll(&wq->wait, EPOLLIN | EPOLLPRI | EPOLLRDNORM | EPOLLRDBAND);
    }
}

void sock_init_data(struct socket * sock, struct sock * sk) {
    if (sock) {
        sock->sk = sk;
        sk->sk_socket = sock;
        sk->sk_wq = &sock->wq;
    }

	sk->sk_state = TCP_CLOSE;
	sk->sk_data_ready = sock_def_readable;
    sk->sk_rcvtimeo = MAX_SCHEDULE_TIMEOUT;
	sk->sk_sndtimeo = MAX_SCHEDULE_TIMEOUT;
    sk->sk_tx_pending = 0;
	sock_set_flag(sk, SOCK_ZAPPED);
    return;
}

void sk_common_release(struct sock * sk) {
    if (sk->sk_prot->destroy) {
        sk->sk_prot->destroy(sk);
    }

	// sk->sk_prot->unhash(sk);
	// sock_put(sk);
    return;
}

struct sock * sk_alloc(int family, struct proto * prot) {
    struct sock * sk;
    sk = (struct sock *)calloc(1, prot->obj_size);
    if (!sk) {
        pr_err("Failed to allocate sock for protocol %s\n", prot->name);
        return NULL;
    }

	memset(sk, 0, prot->obj_size);

    sk->sk_prot = prot;
    sk->sk_family = family;

    return sk;
}