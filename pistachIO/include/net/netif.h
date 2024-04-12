#ifndef _NETIF_H_
#define _NETIF_H_

#include <linux/if_ether.h>

#include "list.h"

struct netif {
    struct list_head list;

    // char name[NETIF_MAX_NAME_LEN];
    int port_id;

    /** link level hardware address of this interface */
    uint8_t hwaddr[ETH_ALEN];

    /** IP address configuration in network byte order */
    uint32_t ipaddr;
    uint32_t netmask;
    uint32_t gw;
};

#endif  /* _NETIF_H_ */