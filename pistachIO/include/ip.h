#ifndef _IP_H_
#define _IP_H_

#include <stdint.h>
#include <arpa/inet.h>
#include <netinet/ip.h>

int ip4_input(uint8_t * pkt, int pkt_size, struct iphdr * iphdr);

#endif  /* _IP_H_ */