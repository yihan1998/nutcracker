#include <stdint.h>

#include "net/netif.h"

struct etharp_entry {
    /* In network byte order */
    uint32_t ip_addr;
    // char name[NETIF_MAX_NAME_LEN];
    uint8_t eth_addr[ETH_ALEN];
    // uint16_t ctime;
    struct netif * netif;
    uint8_t state;
};

struct etharp_entry * etharp_find_entry(uint32_t ipaddr) {
    return NULL;
}
