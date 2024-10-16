/* Automatically generated on Wed Oct 16 12:40:53 2024
 */
#ifndef _NUTCRACKER_GEN_HEADER_
#define _NUTCRACKER_GEN_HEADER_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <rte_common.h>
#include <rte_eal.h>
#include <rte_flow.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_version.h>
#include <doca_flow.h>

#define SET_MAC_ADDR(addr, a, b, c, d, e, f)\
do {\
	addr[0] = a & 0xff;\
	addr[1] = b & 0xff;\
	addr[2] = c & 0xff;\
	addr[3] = d & 0xff;\
	addr[4] = e & 0xff;\
	addr[5] = f & 0xff;\
} while (0)
#endif
