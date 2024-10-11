#include "opt.h"
#include "printk.h"
#include "net/dpdk_module.h"
#include "net/ethernet.h"
#include "net/net.h"
#include "fs/fs.h"

DEFINE_PER_CPU(struct network_info, net_info);

DEFINE_PER_CPU(struct rte_ring *, fwd_queue);

int network_main(void) {
    int pid = 0;
    struct timespec curr;
    struct rte_mbuf * mbuf, * pkts[MAX_PKT_BURST];
    uint8_t * pkt;
    int pkt_size;
    int ret, nb_recv, nb_send;
    struct sk_buff * skb, *skbs[32];

    clock_gettime(CLOCK_REALTIME, &curr);
    if (curr.tv_sec - net_info.last_log.tv_sec >= 1) {
        pr_info("[NET] receive %d packets, send %d packets\n", net_info.sec_recv, net_info.sec_send);
        net_info.sec_recv = net_info.sec_send = 0;
        net_info.last_log = curr;
    }

    pthread_spin_lock(&rx_lock);
    nb_recv = dpdk_recv_pkts(pid, pkts);
    pthread_spin_unlock(&rx_lock);
    if (nb_recv) {
        net_info.sec_recv += nb_recv;
        for (int i = 0; i < nb_recv; i++) {
            pkt = dpdk_get_rxpkt(pid, pkts, i, &pkt_size);
            // fprintf(stderr, "CPU %02d| PKT %p is received\n", sched_getcpu(), pkt);
            ethernet_input(pkts[i], pkt, pkt_size);
        }
    }

	struct list_head pendings = LIST_HEAD_INIT(pendings);
    struct list_head write_skbs = LIST_HEAD_INIT(write_skbs);
#if 0
    pthread_spin_lock(&pending_sk->lock);
    list_splice_tail_init(&pending_sk->head, &pendings);
    pthread_spin_unlock(&pending_sk->lock);

    struct sock * sk, * sk_tmp;
    list_for_each_entry_safe(sk, sk_tmp, &pendings, sk_list) {
        lock_sock(sk);
        list_del_init(&sk->sk_list);
        list_splice_tail_init(&sk->sk_write_queue, &write_skbs);
        unlock_sock(sk);
    }

    struct sk_buff * to_send, * skb_tmp;
    list_for_each_entry_safe(to_send, skb_tmp, &write_skbs, list) {
        list_del_init(&to_send->list);
        mbuf = to_send->mbuf;
        ret = dpdk_insert_txpkt(pid, mbuf);
        if (ret < 0) {
            pr_warn("Run out of TX pkt space\n");
            rte_pktmbuf_free(mbuf);
        }
        free_skb(to_send);
    }
#endif
#if 1
    // int count = 0;
	uint8_t * p;
    struct sock * sk, * sk_tmp;
    pthread_spin_lock(&pending_sk->lock);
    list_for_each_entry_safe(sk, sk_tmp, &pending_sk->head, sk_list) {
        // lock_sock(sk);
        wrlock_sock(sk);
        struct sk_buff * to_send, * skb_tmp;
        // list_for_each_entry(to_send, &sk->sk_write_queue, list) {
        //     fprintf(stderr, "Checking -> skb: %p (<-: %p, ->: %p)\n", to_send, to_send->list.prev, to_send->list.next);
        // }
        list_for_each_entry_safe(to_send, skb_tmp, &sk->sk_write_queue, list) {
            // fprintf(stderr, "Sending skb %p (mbuf: %p)...\n", to_send, to_send->mbuf);
            // fprintf(stderr, "skb: %p (<-: %p, ->: %p), write queue: %p(<-: %p, ->: %p)...\n", to_send, to_send->list.prev, to_send->list.next, &sk->sk_write_queue, sk->sk_write_queue.prev, sk->sk_write_queue.next);
            list_del_init(&to_send->list);
#if 0
            mbuf = to_send->mbuf;
            ret = dpdk_insert_txpkt(pid, mbuf);
            if (ret < 0) {
                pr_err("Run out of TX pkt space\n");
                rte_pktmbuf_free(mbuf);
            }
#endif
            struct rte_mbuf * mbuf = dpdk_alloc_txpkt(to_send->pkt_len);
            if (!mbuf) {
                pr_warn("Out of TX buffer!\n");
                break;
            }

    		p = rte_pktmbuf_mtod(mbuf, uint8_t *);
            memcpy(p, to_send->pkt, to_send->pkt_len);
#if LATENCY_BREAKDOWN
            struct pktgen_tstamp * ts = (struct pktgen_tstamp *)(p + sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr));
            ts->network_send = get_current_time_ns();
#endif
    		dpdk_insert_txpkt(pid, mbuf);
            free_skb(to_send);
        }
        list_del_init(&sk->sk_list);
        unlock_sock(sk);
    }
    pthread_spin_unlock(&pending_sk->lock);
    // if (count) fprintf(stderr, "Write queue has %d packets\n", count);
#endif

    nb_recv = rte_ring_dequeue_burst(fwd_queue, (void **)skbs, 32, NULL);
    if (nb_recv) {
        for (int i = 0; i < nb_recv; i++) {
            skb = skbs[i];
            mbuf = skb->mbuf;
            // pthread_spin_lock(&tx_lock);
            ret = dpdk_insert_txpkt(pid, mbuf);
            // pthread_spin_unlock(&tx_lock);
            if (ret < 0) {
                rte_pktmbuf_free(mbuf);
            }
        }
        rte_mempool_put_bulk(skb_mp, (void *)skbs, nb_recv);
    }

    pthread_spin_lock(&tx_lock);
    nb_send = dpdk_send_pkts(pid);
    pthread_spin_unlock(&tx_lock);
    if (nb_send) {
        net_info.sec_send += nb_send;
        pr_debug(STACK_DEBUG, "%s: send %d packets\n", __func__, nb_send);
    }

    return 0;
}

int pistachio_lcore_init(int lid) {
    char name[RTE_MEMZONE_NAMESIZE];

    sprintf(name, "fwd_queue_%d", lid);
    fwd_queue = rte_ring_create(name, 4096, rte_socket_id(), 0);
    assert(fwd_queue != NULL);

    return 0;
}

int __init pistachio_init(void) {
    // inet_init();
    // packet_init();
    net_init();
    return 0;
}