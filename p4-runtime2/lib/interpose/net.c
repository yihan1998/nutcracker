#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "utils/printk.h"
#include "net/dpdk.h"
#include "net/raw.h"

struct dpdk_config dpdk_config = {
    .port_config.nb_ports = 2,
    .port_config.nb_queues = 1,
    .port_config.nb_hairpin_q = 4,
	.port_config.enable_mbuf_metadata = 1,
};

struct rte_hash * flow_table;

struct rte_mempool * pkt_mempool;

const struct rte_memzone * pending_sk_mz;
struct locked_list_head * pending_sk;

int __init dpdk_init(int argc, char ** argv) {
    int ret;

    if ((ret = rte_eal_init(argc, argv)) < 0) {
        pr_emerg("rte_eal_init() failed! ret: %d\n", ret);
        return -1;
    }

    return 0;
}

int __init raw_init(void) {
    struct rte_tailq_elem raw_pcbs_tailq = {
        .name = "raw_pcbs",
    };

    raw_pcbs = RTE_TAILQ_LOOKUP(raw_pcbs_tailq.name, rte_tailq_entry_head);
    assert(raw_pcbs != NULL);

    return 0;
}

int __init net_init(void) {
    flow_table = rte_hash_find_existing("flow_table");

    pending_sk_mz = rte_memzone_lookup("pending_sk");
	if (pending_sk_mz == NULL) {
		pr_err("Failed to reserve memzone\n");
	}

    pending_sk = pending_sk_mz->addr;

    raw_init();

    return 0;
}