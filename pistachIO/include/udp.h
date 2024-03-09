#ifndef _UDP_H_
#define _UDP_H_

#include "ip.h"

#include <stdint.h>
#include <arpa/inet.h>
#include <netinet/udp.h>

int udp_input(uint8_t * pkt, int pkt_size, struct iphdr * iphdr, struct udphdr * udphdr);

#endif  /* _UDP_H_ */