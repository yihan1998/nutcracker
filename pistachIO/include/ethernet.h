#ifndef _ETHERNET_H_
#define _ETHERNET_H_

#include <stdint.h>
#include <linux/if_ether.h>

int ethernet_input(uint8_t * pkt, int pkt_size);

#endif  /* _ETHERNET_H_ */