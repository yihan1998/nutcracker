#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "opt.h"
#include "utils/printk.h"
#include "net/dpdk.h"
#include "net/sock.h"
#include "net/raw.h"

// struct raw_pcb_head * raw_pcbs;
struct rte_tailq_entry_head * raw_pcbs;

int raw_recvmsg(struct sock * sk, struct msghdr * msg, size_t len, int flags) {
	// struct raw_pcb * pcb = &sk->pcb.raw;
	// struct sk_buff * skb, * next;
	// struct list_head recv_skbs = LIST_HEAD_INIT(recv_skbs);
    // lock_sock(sk);
    // list_splice_tail_init(&pcb->recv_skbs, &recv_skbs);
    // unlock_sock(sk);
    // int cnt = 0;
    // list_for_each_entry_safe(skb, next, &recv_skbs, list) {
    //     cnt++;
    // }

	struct sk_buff * skb;
	int copied, err;
	err = -EINVAL;

	skb = skb_recv_datagram(sk, flags, flags & MSG_DONTWAIT, &err);

    if (skb == NULL) {
		goto out;
    }

    copied = skb->data_len;
	if (copied > len) {
		copied = len;
		msg->msg_flags |= MSG_TRUNC;
	}

	err = skb_copy_datagram_msg(skb, 0, msg, copied);
	if (err) {
		goto out_free;
    }

	err = ((flags & MSG_TRUNC) ? skb->data_len : copied);

out_free:
	// skb_free_datagram(sk, skb);
	rte_pktmbuf_free(skb->mbuf);
    free_skb(skb);
out:
	return err;
}

int init_raw_pcb(struct raw_pcb * pcb, uint8_t proto) {
	pcb->protocol = proto;
    pcb->ttl = IP_DEFAULT_TTL;
    // pcb->list.data = pcb;
	// TAILQ_INSERT_TAIL(raw_pcbs, &pcb->list, next);
	return 0;
}
