#include "fs/fs.h"
#include "net/net.h"
#include "net/skbuff.h"

struct sk_buff * alloc_skb(struct rte_mbuf * mbuf, uint8_t * pkt, unsigned int pkt_len) {
    struct sk_buff * skb = NULL;

    pthread_spin_lock(skb_mp_lock);
    rte_mempool_get(skb_mp, (void *)&skb);
    pthread_spin_unlock(skb_mp_lock);
    if (!skb) {
        return NULL;
    }

    init_list_head(&skb->list);
    skb->pkt_len = pkt_len;
    if (pkt && mbuf) {
        skb->pkt = pkt;
        skb->mbuf = mbuf;
    } else {
        skb->pkt = skb->buff;
        skb->mbuf = NULL;
    }

    return skb;
}

void free_skb(struct sk_buff * skb) {
    // fprintf(stderr, "Releasing skb %p (<-: %p, ->: %p)\n", skb, skb->list.prev, skb->list.next);
    // memset(skb, 0, sizeof(struct sk_buff));
    pthread_spin_lock(skb_mp_lock);
    rte_mempool_put(skb_mp, skb);
    pthread_spin_unlock(skb_mp_lock);
}