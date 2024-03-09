#ifndef _CORE_H_
#define _CORE_H_

#include <stdbool.h>

#include "init.h"

#ifndef CONFIG_NR_CPUS
#define CONFIG_NR_CPUS  1
#endif

#define NR_CPUS CONFIG_NR_CPUS

/* Per-core information */
struct core_info {
    int sockfd;
    bool occupied;
};

extern struct core_info core_infos[NR_CPUS];

extern unsigned int cpu_id;

extern int sched_accept_worker(int epfd);

extern int __init sched_init(void);

#endif  /* _CORE_H_ */