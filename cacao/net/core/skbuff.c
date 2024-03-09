#include "kernel.h"
#include "skbuff.h"

#define SHM_SIZE    8192

struct sk_buff_pool * sk_buff_pool_init(int sockfd, int nb_element) {
    char name[32];
    sprintf(name, "sk_buff_pool_%d", sockfd);

    /* Calculate the aligned size of sk_buff element */
    size_t elt_size = ALIGN_CEIL(sizeof(struct sk_buff) + 1500, 16);
    /* Calculate the total size of sk_buff_pool */
    size_t tot_size = ALIGN_CEIL(sizeof(struct sk_buff_pool) + elt_size * nb_element, 16);

    int shm_fd = shm_open("sk_buff_pool", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return NULL;
    }

    // Size the shared memory object
    if (ftruncate(shm_fd, tot_size) == -1) {
        perror("ftruncate");
        close(shm_fd);
        shm_unlink(name);
        return NULL;
    }

    // Map the shared memory object
    void * ptr = mmap(0, tot_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        shm_unlink(name);
        return NULL;
    }

    struct sk_buff_pool * pool = (struct sk_buff_pool *)ptr;

    pool->nb_element = nb_element;
    init_list_head(&pool->free);

    uint8_t * p = pool->elements;

    for (int i = 0; i < nb_element; i++) {
        struct sk_buff * element = (struct sk_buff *)p;

        list_add_tail(&pool->free, &element->list);
        element->len = 1500;

        p += (sizeof(struct sk_buff) + 1500);
    }

    return pool;
}

struct sk_buff * sk_buff_malloc(struct sk_buff_pool * pool) {
	return list_first_entry_or_null(&pool->free, struct sk_buff, list);
}

void sk_buff_free(struct sk_buff_pool * pool, struct sk_buff * skb) {
	return list_add_tail(&pool->free, &skb->list);
}
