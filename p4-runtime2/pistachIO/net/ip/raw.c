#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <rte_tailq.h>

#include "opt.h"
#include "utils/printk.h"
#include "kernel.h"
#include "net/sock.h"
#include "net/raw.h"

static uint8_t raw_input_local_match(struct sk_buff * skb, struct raw_pcb * pcb, uint8_t broadcast) {
	// if (broadcast != 0) {
	// 	if (ip4_addr_isany(&pcb->local_ip)) {
	// 		return 1;
	// 	}
	// } else if (ip4_addr_isany(&pcb->local_ip) ||
	// 	ip4_addr_eq(&pcb->local_ip, IPCB(skb)->daddr)) {
	// 	return 1;
	// }

  	// return 0;
	/* TODO: bind to one IP address */
	return 1;
}

raw_input_state_t raw_input(struct sk_buff * skb) {
	raw_input_state_t ret = RAW_INPUT_NONE;
	uint8_t broadcast = true;
	struct rte_tailq_entry * entry;

	TAILQ_FOREACH(entry, raw_pcbs, next) {
		struct raw_pcb * pcb = (struct raw_pcb *)entry->data;
		pr_debug(RAW_DEBUG, "Checking proto -> receive: %u, registered: %u\n", IPCB(skb)->proto, pcb->protocol);
		if ((pcb->protocol == IPCB(skb)->proto) && raw_input_local_match(skb, pcb, broadcast)) {
			ret = RAW_INPUT_DELIVERED;
			/* the receive callback function did not eat the packet? */
			// eaten = pcb->recv(pcb->recv_arg, pcb, p, ip_current_src_addr());
			struct sock * sk = __container_of__(pcb, struct sock, pcb.raw);
			/* A lot more to do but im too tired to do... */
			// lock_sock(sk);
		    wrlock_sock(sk);
			list_add_tail(&skb->list, &sk->sk_receive_queue);
			unlock_sock(sk);
			return RAW_INPUT_EATEN;
		}
	}

  	return ret;
}

int __init raw_init(void) {
    // size_t size = sizeof(struct raw_pcb_head);

    // int fd = shm_open("raw_pcbs", O_CREAT | O_RDWR, 0666);
    // if (fd == -1) {
    //     perror("shm_open");
    //     exit(EXIT_FAILURE);
    // }

	// if (ftruncate(fd, size) == -1) {
    //     perror("ftruncate");
    //     exit(EXIT_FAILURE);
    // }

    // raw_pcbs = (struct raw_pcb_head *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	// if (raw_pcbs == MAP_FAILED) {
    //     perror("mmap");
    //     exit(EXIT_FAILURE);
    // }

	// if (pthread_rwlock_init(&raw_pcbs->rwlock, NULL) != 0) {
    //     perror("pthread_rwlock_init");
    //     exit(EXIT_FAILURE);
    // }
	
	struct rte_tailq_elem raw_pcbs_tailq = {
        .name = "raw_pcbs",
    };

	if ((rte_eal_tailq_register(&raw_pcbs_tailq) < 0) || (raw_pcbs_tailq.head == NULL)) {
		pr_err("Error allocating %s\n", raw_pcbs_tailq.name);
    }
    raw_pcbs = RTE_TAILQ_CAST(raw_pcbs_tailq.head, rte_tailq_entry_head);
    assert(raw_pcbs != NULL);

    return 0;
}