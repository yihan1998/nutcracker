#ifndef _INET_SOCK_H_
#define _INET_SOCK_H_

#include "net/sock.h"

struct inet_sock {
    struct sock sk;
#define inet_daddr		sk.sk_common.skc_daddr
#define inet_rcv_saddr  sk.sk_common.skc_rcv_saddr
#define inet_dport		sk.sk_common.skc_dport
#define inet_num		sk.sk_common.skc_num
    /* Network byte order */
    uint32_t inet_saddr;
    /* Network byte order */
    uint16_t inet_sport;
    unsigned char is_icsk;
};

static inline struct inet_sock * inet_sk(const struct sock * sk) {
	return (struct inet_sock *)sk;
}

void inet_sk_set_state(struct sock * sk, int state);

#endif  /* _INET_SOCK_H_ */