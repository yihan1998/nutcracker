#ifndef _THREADS_H_
#define _THREADS_H_

#include <pthread.h>

#include "init.h"
#include "percpu.h"
#ifdef CONFIG_DOCA_REGEX
#include "doca/regex.h"
#endif  /* CONFIG_DOCA */

#ifndef CONFIG_NR_CPUS
#define CONFIG_NR_CPUS  1
#endif

#define NR_CPUS CONFIG_NR_CPUS

extern pthread_t worker_ids[NR_CPUS];


struct worker_context {
#ifdef CONFIG_DOCA
	struct doca_workq * workq;   /* DOCA work queue */
#ifdef CONFIG_DOCA_REGEX
    struct doca_regex_ctx regex_ctx;
#endif  /* CONFIG_DOCA_REGEX */
#endif  /* CONFIG_DOCA */
};

extern struct worker_context * contexts[NR_CPUS];
DECLARE_PER_CPU(struct worker_context *, worker_ctx);

extern int __init worker_init(void);

#endif  /* _THREADS_H_ */