#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "net/sock.h"

int skb_copy_datagram_msg(struct sk_buff * from, int offset, struct msghdr * msg, int size) {
    memcpy(msg->msg_iov->iov_base, skb_payload(from), size);
    return 0;
}


int skb_wait_for_more_packets(struct sock * sk, int * err, long * timeo_p) {
	int error;
	// DEFINE_WAIT_FUNC(wait, receiver_wake_function);
	// prepare_to_wait_exclusive(sk_sleep(sk), &wait, TASK_INTERRUPTIBLE);

    // bool empty = false;

    do {
        // usleep(5);
        // rdlock_sock(sk);
        // empty = list_empty(&sk->sk_receive_queue);
        // unlock_sock(sk);
        // if (empty) continue;
        // else break;
    } while (skb_queue_empty_lockless(&sk->sk_receive_queue));

	error = 0;
	// *timeo_p = schedule_timeout(*timeo_p);

	// finish_wait(sk_sleep(sk), &wait);
	return error;
}

struct sk_buff * skb_try_recv_datagram(struct sock * sk, unsigned int flags, int * err) {
    int error = 0;
    struct list_head * queue = &sk->sk_receive_queue;
	struct sk_buff * skb;

    /* Again only user level code calls this function, so nothing
        * interrupt level will suddenly eat the receive_queue.
        *
        * Look at current nfs client by the way...
        * However, this function was correct in any case. 8)
        */
    // lock_sock(sk);
    wrlock_sock(sk);
    skb = skb_try_recv_from_queue(sk, queue, flags, &error);
    // unlock_sock(sk);
    unlock_sock(sk);
    if (error) {
        goto no_packet;
    }
    if (skb) {
        return skb;
    }

	error = -EAGAIN;

no_packet:
	*err = error;
	return NULL;
}

struct sk_buff * skb_recv_datagram(struct sock * sk, unsigned int flags, int noblock, int * err) {
	struct sk_buff * skb;
    do {
		skb = skb_try_recv_datagram(sk, flags, err);
		if (skb) {
			return skb;
        }

		if (*err != -EAGAIN) {
			break;
        }
	} while (!skb_wait_for_more_packets(sk, err, NULL));

	return NULL;
}

struct sk_buff * skb_try_recv_from_queue(struct sock * sk, struct list_head * queue, unsigned int flags, int * err) {
    struct sk_buff * skb = NULL;

    skb = list_first_entry_or_null(queue, struct sk_buff, list);
    if (!skb) {
        return NULL;
    }

    list_del_init(&skb->list);
#if LATENCY_BREAKDOWN
        struct pktgen_tstamp * ts = (struct pktgen_tstamp *)skb_payload(skb);
        ts->server_recv = get_current_time_ns();
#endif
    // fprintf(stderr, "Dequeue skb %p (<-: %p, ->: %p) to write queue\n", skb, skb->list.prev, skb->list.next);

    return skb;
}