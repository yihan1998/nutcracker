#ifndef _TCP_H_
#define _TCP_H_

#include "ip.h"

#include <stdint.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

int tcp_input(uint8_t * pkt, int pkt_size, struct iphdr * iphdr, struct tcphdr * tcphdr);

#endif  /* _TCP_H_ */