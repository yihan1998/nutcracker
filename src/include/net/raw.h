#ifndef _RAW_H_
#define _RAW_H_

#include <stdint.h>
#include <pthread.h>
#include <rte_tailq.h>

#include "list.h"
#include "net/ip.h"
#include "net/net.h"
#include "net/skbuff.h"

// struct raw_pcb_head {
//     TAILQ_HEAD(tailq_head, raw_pcb) head;
//     pthread_rwlock_t rwlock;
// };

/** return codes for raw_input */
typedef enum raw_input_state {
	RAW_INPUT_NONE = 0, /* pbuf did not match any pcbs */
	RAW_INPUT_EATEN,    /* pbuf handed off and delivered to pcb */
	RAW_INPUT_DELIVERED /* pbuf only delivered to pcb (pbuf can still be referenced) */
} raw_input_state_t;

extern raw_input_state_t raw_input(struct sk_buff * skb);

extern int raw_recvmsg(struct sock * sk, struct msghdr * msg, size_t len, int flags);

// extern struct raw_pcb_head * raw_pcbs;
extern struct rte_tailq_entry_head * raw_pcbs;

/** the RAW protocol control block */
struct raw_pcb {
	/* Common members of all PCB types */
	IP_PCB;

	uint8_t	protocol;
	uint8_t flags;
};

extern int init_raw_pcb(struct raw_pcb * pcb, uint8_t proto);
extern int raw_init(void);

#endif  /* _RAW_H_ */