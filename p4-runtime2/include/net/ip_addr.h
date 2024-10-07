#ifndef _IP_ADDR_H_
#define _IP_ADDR_H_

#define IPADDR_ANY  ((uint32_t)0x00000000UL)

#define ip4_addr_isany_val(addr1)   ((addr1) == IPADDR_ANY)
#define ip4_addr_isany(addr1)       ((addr1) == NULL || ip4_addr_isany_val(*(addr1)))

#endif  /* _IP_ADDR_H_ */