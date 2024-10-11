#define _GNU_SOURCE
#include <stdlib.h>
#include <sys/epoll.h>
#include <sched.h>

#include "opt.h"
#include "printk.h"
#include "kernel/wait.h"
#include "fs/fs.h"
#include "net/net.h"
#include "net/sock.h"
#include "net/tcp_states.h"

void sock_init_data(struct socket * sock, struct sock * sk) {
    init_list_head(&sk->sk_list);
    init_list_head(&sk->sk_receive_queue);
    init_list_head(&sk->sk_write_queue);
    return;
}

void sk_common_release(struct sock * sk) {
    return;
}

struct sock * sk_alloc(int family) {
    struct sock * sk;

    rte_mempool_get(sock_mp, (void *)&sk);
    if (!sk) {
        return NULL;
    }

    sk->sk_family = family;

    return sk;
}