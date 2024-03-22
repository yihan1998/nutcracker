#ifndef _UDP_H_
#define _UDP_H_

#include <stdint.h>
#include <arpa/inet.h>
#include <netinet/udp.h>

#include "list.h"
#include "net/ip.h"
#include "net/inet_sock.h"

struct udp_sock {
	struct inet_sock inet;

	struct list_head receive_queue;
	struct list_head transmit_queue;
};

static inline struct udp_sock * udp_sk(const struct sock * sk) {
	return (struct udp_sock *)sk;
}

extern struct proto udp_prot;

extern __poll_t udp_poll(struct file * file, struct socket * sock, poll_table * wait);
extern int udp_input(uint8_t * pkt, int pkt_size, struct iphdr * iphdr, struct udphdr * udphdr);
extern int udp_output(struct sock * sk);

#endif  /* _UDP_H_ */