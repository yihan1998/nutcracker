#ifndef _TCP_H_
#define _TCP_H_

#include "net/ip.h"

#include <stdint.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

extern int tcp_input(struct sk_buff * skb, struct iphdr * iphdr, struct tcphdr * tcphdr);

#endif  /* _TCP_H_ */