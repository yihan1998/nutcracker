#include <assert.h>

#include "opt.h"
#include "printk.h"
#include "net/net.h"
#include "net/flow.h"
#include "net/sock.h"

DEFINE_PER_CPU(struct rte_hash *, udp_table);

static struct rte_hash_parameters params = {
	.entries = 512,
	.key_len = sizeof(struct flowi4),
	.hash_func = rte_jhash,
	.hash_func_init_val = 0,
	.socket_id = 0,
};

int __init net_init(void) {
    char name[32];

	pr_info("init: register INET domain...\n");
	inet_init();

	/* Unit UNIX socket because we need it for message passing */
    pr_info("init: register UNIX domain...\n");
    unix_init();

    sprintf(name, "udp_table_%d", cpu_id);
	udp_table = rte_hash_find_existing("udp_table");
    assert(udp_table != NULL);

    return 0;
}
