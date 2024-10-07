#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>

#include "opt.h"
#include "utils/printk.h"
#include "kernel.h"
#include "fs/fs.h"
#include "net/dpdk.h"
#include "net/netfmt.h"
#include "net/net.h"
#include "net/skbuff.h"
#include "net/flow.h"
#include "net/sock.h"
#include "net/ethernet.h"
#include "net/udp.h"

#define UDP_LOCAL_PORT_RANGE_START  0xc000
#define UDP_LOCAL_PORT_RANGE_END    0xffff

static uint16_t udp_new_port(void) {
    return 0;
}

int udp_bind(struct sock * sk, const struct sockaddr * addr, int addrlen) {
	int pos;
    struct sockaddr_in * in = (struct sockaddr_in *)addr;
    uint16_t port = in->sin_port;   /* In network byte order */
    struct flowi4 key = {0};
	key.saddr 		= sk->sk_rcv_saddr;
	key.daddr 		= 0;
	key.fl4_sport 	= 0;
	key.fl4_dport 	= 0;
	key.flowi4_proto 	= IPPROTO_UDP;

    /* no port specified? */
    if (port == 0) {
        pr_debug(UDP_DEBUG, "allocating new port...\n");
        port = udp_new_port();
        if (port == 0) {
            /* no more ports available in local range */
            pr_debug(UDP_DEBUG, "out of free UDP ports\n");
            return -EADDRINUSE;
        }
    } else {
        /* Check if the port has been binded */
        key.fl4_sport = port;
        pr_debug(UDP_DEBUG, "saddr: " IP_STRING ", sport: %u, daddr: " IP_STRING ", dport: %u\n",
        			    NET_IP_FMT(key.saddr), ntohs(key.fl4_sport), NET_IP_FMT(key.daddr), ntohs(key.fl4_dport));

    	hash_sig_t hash_value = rte_jhash(&key, sizeof(struct flowi4), 0);
		pos = rte_hash_add_key_with_hash_data(flow_table, (const void *)&key, hash_value, (void *)sk);
		if (pos < 0) {
			pr_warn("Failed to add key (pos0=%d)", pos);
		}

        sk->pcb.udp.local_port = port;

		return 0;
    }
    
    return -1;
}

struct sk_buff * skb_recv_udp(struct sock * sk, unsigned int flags, int * err) {
	struct list_head * sk_queue = &sk->sk_receive_queue;
    struct sk_buff * skb;
	int error = EAGAIN;

    do {
		error = -EAGAIN;

        // lock_sock(sk);
        wrlock_sock(sk);
        skb = skb_try_recv_from_queue(sk, sk_queue, flags, err);
        unlock_sock(sk);

#if 1
        if (skb) {
            pr_debug(UDP_DEBUG, "receiving skb %p\n", skb);
            return skb;
        }
#endif
#if 0
        if (skb) {
            struct sk_buff * new_skb;
            pthread_spin_lock(skb_mp_lock);
            rte_mempool_get(skb_mp, (void *)&new_skb);
            pthread_spin_unlock(skb_mp_lock);
            if (!new_skb) {
                pr_warn("Failed to allocate new skb!\n");
                return NULL;
            }

            init_list_head(&new_skb->list);
            new_skb->sk = skb->sk;
            new_skb->mbuf = NULL;
            new_skb->pkt_len = new_skb->pkt_len;
            new_skb->data_start = skb->data_start;
            new_skb->transport_header = skb->transport_header;
            new_skb->network_header = skb->network_header;
            new_skb->mac_header = skb->mac_header;
            new_skb->data_len = skb->data_len;

            memcpy(new_skb->buff, skb->buff, skb->pkt_len);

            pthread_spin_lock(&pending_sk->lock);
            lock_sock(sk);
            list_add_tail(&new_skb->list, &sk->sk_write_queue);
            // fprintf(stderr, "Release old skb %p (<-: %p, ->: %p) from receive queue: %p(<-: %p, ->: %p)\nAdd new skb %p (<-: %p, ->: %p) to , write queue: %p(<-: %p, ->: %p)...\n", skb, skb->list.prev, skb->list.next, &sk->sk_receive_queue, sk->sk_receive_queue.prev, sk->sk_receive_queue.next, new_skb, new_skb->list.prev, new_skb->list.next, &sk->sk_write_queue, sk->sk_write_queue.prev, sk->sk_write_queue.next);
            if (list_empty(&sk->sk_list)) {
                list_add_tail(&sk->sk_list, &pending_sk->head);
            }
            unlock_sock(sk);
            pthread_spin_unlock(&pending_sk->lock);

            free_skb(skb);
        }
#endif
	} while (!skb_wait_for_more_packets(sk, &error, NULL));

	*err = error;
	return NULL;
}

int udp_sendmsg(struct sock *sk, struct msghdr * msg, size_t len) {
    struct sk_buff * skb;
    struct udp_pcb * pcb = &sk->pcb.udp;
    uint16_t data_len, tlen;
	struct sockaddr_in * usin = (struct sockaddr_in *)msg->msg_name;
    struct ethhdr * ethhdr;
    struct iphdr * iphdr;
    struct udphdr * udphdr;
    // uint8_t * data;
    uint32_t daddr = 0;
	uint16_t dport = 0;

#if LATENCY_BREAKDOWN
    struct pktgen_tstamp * ts = (struct pktgen_tstamp *)msg->msg_iov->iov_base;
    ts->server_sendto = get_current_time_ns();
#endif

    data_len = msg->msg_iov->iov_len;

    tlen = data_len + sizeof(struct udphdr) + sizeof(struct iphdr) + sizeof(struct ethhdr);

#if 1
    // mbuf = rte_pktmbuf_alloc(pkt_mempool);
    // if (!mbuf) {
    //     pr_warn("Failed to allocate mbuf!\n");
    //     return -1;
    // }

    // p = rte_pktmbuf_mtod(mbuf, uint8_t *);

    /* The len is the payload len, should be updated later with header */
    skb = alloc_skb(NULL, NULL, tlen);
    if (!skb) {
        pr_warn("Failed to allocate skb!\n");
        goto out;
    }
#endif

#if 0
    struct rte_mbuf * mbuf;
    uint8_t * p;
    mbuf = rte_pktmbuf_alloc(pkt_mempool);
    if (!mbuf) {
        pr_warn("Failed to allocate mbuf!\n");
        return -1;
    }

    p = rte_pktmbuf_mtod(mbuf, uint8_t *);
    skb = alloc_skb(mbuf, p, tlen);
    if (!skb) {
        pr_warn("Failed to allocate skb!\n");
        goto out;
    }
#endif

    /*
	 *	Get and verify the address.
	 */
	if (usin) {
		if (msg->msg_namelen < sizeof(*usin))
			return -EINVAL;
		if (usin->sin_family != AF_INET) {
			if (usin->sin_family != AF_UNSPEC)
				return -EAFNOSUPPORT;
		}

		daddr = usin->sin_addr.s_addr;
		dport = usin->sin_port;
		if (dport == 0)
			return -EINVAL;
	}

    skb->mac_header = 0;
    skb->network_header = sizeof(struct ethhdr);
    skb->transport_header = sizeof(struct ethhdr) + sizeof(struct iphdr);
    skb->data_start = sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr);

    ethhdr = eth_hdr(skb);
    iphdr = ip_hdr(skb);
    udphdr = udp_hdr(skb);
    skb->data_len = data_len;

    memcpy(skb_payload(skb), msg->msg_iov->iov_base, msg->msg_iov->iov_len);

    /* UDP */
    iphdr->saddr = pcb->local_ip;
    iphdr->daddr = daddr;
    iphdr->version = 4;
    iphdr->ihl = (sizeof(struct iphdr) / 4);

    tlen = data_len + sizeof(struct udphdr) + sizeof(struct iphdr);
    iphdr->tot_len = htons(tlen);
    iphdr->protocol = IPPROTO_UDP;

    tlen = data_len + sizeof(struct udphdr);
    udphdr->len     = htons(tlen);
    udphdr->source  = pcb->local_port;
    udphdr->dest    = dport;
    udphdr->check   = 0;

    /* IP */
    tlen = data_len + sizeof(struct udphdr) + sizeof(struct iphdr);
    iphdr->tot_len  = htons(tlen);
    iphdr->ttl      = IP_DEFAULT_TTL;
    iphdr->tos      = 0;

    iphdr->id       = htons(17);
    iphdr->frag_off = 0;
    iphdr->protocol = IPPROTO_UDP;
    iphdr->saddr    = pcb->local_ip;
    iphdr->daddr    = daddr;
    iphdr->check    = 0;

    pr_debug(UDP_DEBUG, "[%s:%d] saddr: " IP_STRING ", sport: %u, daddr: " IP_STRING ", dport: %u\n",
                    __func__, __LINE__, NET_IP_FMT(iphdr->saddr), ntohs(udphdr->source), NET_IP_FMT(iphdr->daddr), ntohs(udphdr->dest));

    /* Ethernet */
    ethhdr->h_proto = htons(ETH_P_IP);
    ETHADDR_COPY(&ethhdr->h_source, &src_addr);
    ETHADDR_COPY(&ethhdr->h_dest, &dst_addr);

#if LATENCY_BREAKDOWN
    ts = (struct pktgen_tstamp *)skb_payload(skb);
    ts->network_udp_output = get_current_time_ns();
#endif

#if 0
    mbuf->pkt_len = mbuf->data_len = skb->pkt_len;
    mbuf->nb_segs = 1;
    mbuf->next = NULL;
#endif
    // lock_sock(sk);
    wrlock_sock(sk);
    list_add_tail(&skb->list, &sk->sk_write_queue);
    unlock_sock(sk);

    pthread_spin_lock(&pending_sk->lock);
    // lock_sock(sk);
    wrlock_sock(sk);
    if (list_empty(&sk->sk_list)) {
    	list_add_tail(&sk->sk_list, &pending_sk->head);
    }
    unlock_sock(sk);
    pthread_spin_unlock(&pending_sk->lock);

    return 0;

out:
    return -1;
}

int udp_recvmsg(struct sock * sk, struct msghdr * msg, size_t len, int flags, int * addr_len) {
	struct sk_buff * skb;
    int err;
	unsigned int ulen, copied;
	struct sockaddr_in * sin = (struct sockaddr_in *)msg->msg_name;

	skb = skb_recv_udp(sk, flags, &err);
    if (!skb) {
		return err;
    }

    pr_debug(UDP_DEBUG, "UDP recvmsg -> sk %p saddr: " IP_STRING ", sport: %u, daddr: " IP_STRING ", dport: %u, data len: %u\n",
            skb, NET_IP_FMT(ip_hdr(skb)->saddr), ntohs(udp_hdr(skb)->source), 
            NET_IP_FMT(ip_hdr(skb)->daddr), ntohs(udp_hdr(skb)->dest), skb->data_len);

	ulen = skb->data_len;
	copied = msg->msg_iov->iov_len;
	if (copied > ulen) {
		copied = ulen;
    } else if (copied < ulen) {
		msg->msg_flags |= MSG_TRUNC;
    }

    err = skb_copy_datagram_msg(skb, 0, msg, copied);

    /* Copy the address. */
	if (sin) {
		sin->sin_family = AF_INET;
		sin->sin_port = udp_hdr(skb)->source;
		sin->sin_addr.s_addr = ip_hdr(skb)->saddr;
		memset(sin->sin_zero, 0, sizeof(sin->sin_zero));
		*addr_len = sizeof(*sin);
	}

    // rte_pktmbuf_free(skb->mbuf);
    free_skb(skb);

    err = copied;

#if LATENCY_BREAKDOWN
    struct pktgen_tstamp * ts = (struct pktgen_tstamp *)msg->msg_iov->iov_base;
    ts->server_recvfrom = get_current_time_ns();
#endif

	return err;
}

int init_udp_pcb(struct udp_pcb * pcb) {
    return 0;
}
