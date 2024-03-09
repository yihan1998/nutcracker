#ifndef _SKBUFF_H_
#define _SKBUFF_H_

#include <sys/mman.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

#include "list.h"

struct sk_buff {
    struct list_head list;
    int len;
    char data[0];
};

struct sk_buff_pool {
    size_t      total_size;

    // pthread_spin_lock lock;

    struct list_head free;

    uint32_t    nb_element;
    uint8_t     elements[0];
};

extern struct sk_buff_pool * sk_buff_pool_init(int sockfd, int nb_element);
extern struct sk_buff * sk_buff_malloc(struct sk_buff_pool * pool);
extern void sk_buff_free(struct sk_buff_pool * pool, struct sk_buff * skb);

#endif  /* _SKBUFF_H_ */