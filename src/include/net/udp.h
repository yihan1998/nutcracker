#ifndef _UDP_H_
#define _UDP_H_

#include <stdint.h>
#include <arpa/inet.h>
#include <netinet/udp.h>

#include "list.h"
#include "net/ip.h"

/** the UDP protocol control block */
struct udp_pcb {
/** Common members of all PCB types */
    IP_PCB;

    uint8_t flags;
/** ports are in host byte order */
    uint16_t local_port;
    uint16_t remote_port;
};

static inline struct udphdr * udp_hdr(const struct sk_buff * skb) {
	return (struct udphdr *)skb_transport_header(skb);
}

extern __poll_t udp_poll(struct file * file, struct socket * sock, poll_table * wait);
extern int udp_input(struct sk_buff * skb, struct iphdr * iphdr, struct udphdr * udphdr);
extern int udp_output(struct sock * sk);

extern int udp_bind(struct sock * sk, const struct sockaddr * addr, int addrlen);
extern int udp_sendmsg(struct sock *sk, struct msghdr * msg, size_t len);
extern int udp_recvmsg(struct sock * sk, struct msghdr * msg, size_t len, int flags, int * addr_len);
extern int init_udp_pcb(struct udp_pcb * pcb);

#endif  /* _UDP_H_ */