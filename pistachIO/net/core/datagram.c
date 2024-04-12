#include <stdbool.h>
#include <sys/epoll.h>
#include <sys/uio.h>

#include "kernel/wait.h"
#include "net/skbuff.h"
#include "net/sock.h"
#include "net/udp.h"

__poll_t datagram_poll(struct file * file, struct socket * sock, struct poll_table_struct * wait) {
	struct sock * sk = sock->sk;
	struct udp_sock * udp = udp_sk(sk);
	__poll_t mask = 0;

	sock_poll_wait(file, sock, wait);

	if (!list_empty(&udp->receive_queue)) {
		mask |= EPOLLIN;
	}

    // if (sock_writeable(sk)) {
	// 	mask |= EPOLLOUT;
	// }

	return mask;
}

struct sk_buff * __skb_try_recv_from_queue(struct sock * sk, unsigned int flags, int * err) {
	return list_first_entry_or_null(&udp_sk(sk)->receive_queue, struct sk_buff, list);
}

int __skb_wait_for_more_packets(struct sock * sk, int * err, long * timeo_p) {
	// struct udp_sock * udp = udp_sk(sk);
	return 0;
}

/**
 *	skb_copy_datagram_iter - Copy a datagram to an iovec iterator.
 *	@skb: buffer to copy
 *	@to: iovec iterator to copy to
 *	@len: amount of data to copy from buffer to iovec
 */
static int skb_copy_datagram_iter(struct sk_buff * skb, struct iovec * to, int len) {
	void * dst_buf = to->iov_base;
	memcpy(dst_buf, skb->buf, len);
	return len;
}

int skb_copy_datagram_msg(struct sk_buff * from, struct msghdr * msg, int size) {
	return skb_copy_datagram_iter(from, msg->msg_iov, size);
}
