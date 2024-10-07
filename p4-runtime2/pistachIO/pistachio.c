#include "opt.h"
#include "utils/printk.h"
#include "fs/fs.h"
#include "net/dpdk.h"
#include "net/ethernet.h"
#include "net/net.h"

DEFINE_PER_CPU(struct network_info, net_info);
DEFINE_PER_CPU(struct rte_ring *, fwd_queue);

int net_loop(void) {
    struct timespec curr;
    uint8_t * pkt;
    int pkt_size;
    struct rte_mbuf * mbuf, * pkts[MAX_PKT_BURST];
    int ret;
    int nb_recv = 0, nb_send = 0;
	int portid;
    struct sk_buff * skb, *skbs[32];

    clock_gettime(CLOCK_REALTIME, &curr);
    if (curr.tv_sec - net_info.last_log.tv_sec >= 1) {
        pr_info("[NET] receive %d packets, send %d packets\n", net_info.sec_recv, net_info.sec_send);
        net_info.sec_recv = net_info.sec_send = 0;
        net_info.last_log = curr;
    }

	RTE_ETH_FOREACH_DEV(portid) {
        clock_gettime(CLOCK_REALTIME, &curr);
        if (curr.tv_sec - net_info.last_log.tv_sec >= 1) {
            pr_info("[NET] receive %d packets, send %d packets\n", net_info.sec_recv, net_info.sec_send);
            net_info.sec_recv = net_info.sec_send = 0;
            net_info.last_log = curr;
        }

        pthread_spin_lock(&rx_lock);
        nb_recv = dpdk_recv_pkts(portid, pkts);
        pthread_spin_unlock(&rx_lock);
        if (nb_recv) {
            net_info.sec_recv += nb_recv;
            for (int i = 0; i < nb_recv; i++) {
                pkt = dpdk_get_rxpkt(portid, pkts, i, &pkt_size);
                // fprintf(stderr, "CPU %02d| PKT %p is received\n", sched_getcpu(), pkt);
                ethernet_input(pkts[i], pkt, pkt_size);
            }
        }

	    struct list_head pendings = LIST_HEAD_INIT(pendings);
        struct list_head write_skbs = LIST_HEAD_INIT(write_skbs);

        uint8_t * p;
        struct sock * sk, * sk_tmp;
        pthread_spin_lock(&pending_sk->lock);
        list_for_each_entry_safe(sk, sk_tmp, &pending_sk->head, sk_list) {
            // lock_sock(sk);
            wrlock_sock(sk);
            struct sk_buff * to_send, * skb_tmp;
            list_for_each_entry_safe(to_send, skb_tmp, &sk->sk_write_queue, list) {
                list_del_init(&to_send->list);
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
                dpdk_insert_txpkt(portid, mbuf);
                free_skb(to_send);
            }
            list_del_init(&sk->sk_list);
            unlock_sock(sk);
        }
        pthread_spin_unlock(&pending_sk->lock);

        nb_recv = rte_ring_dequeue_burst(fwd_queue, (void **)skbs, 32, NULL);
        if (nb_recv) {
            for (int i = 0; i < nb_recv; i++) {
                skb = skbs[i];
                mbuf = skb->mbuf;
                ret = dpdk_insert_txpkt(portid, mbuf);
                if (ret < 0) {
                    rte_pktmbuf_free(mbuf);
                }
            }
            rte_mempool_put_bulk(skb_mp, (void *)skbs, nb_recv);
        }

        pthread_spin_lock(&tx_lock);
        nb_send = dpdk_send_pkts(portid);
        pthread_spin_unlock(&tx_lock);
        if (nb_send) {
            net_info.sec_send += nb_send;
            pr_debug(STACK_DEBUG, "%s: send %d packets\n", __func__, nb_send);
        }
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
    pr_info("\tinit: initializing net...\n");
    net_init();
    return 0;
}