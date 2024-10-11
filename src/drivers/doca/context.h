#ifndef _DOCA_CONTEXT_H_
#define _DOCA_CONTEXT_H_

#include "init.h"
#include "percpu.h"

#ifdef CONFIG_DOCA

#include "doca/compress.h"

#define WORKQ_DEPTH	128

struct docadv_worker_context {
#ifdef CONFIG_DOCA
	struct doca_workq * workq;   /* DOCA work queue */
#ifdef CONFIG_DOCA_REGEX
    struct regex_ctx regex_ctx;
#endif  /* CONFIG_DOCA_REGEX */
#ifdef CONFIG_DOCA_COMPRESS
    struct docadv_compress_ctx compress_ctx;
#endif  /* CONFIG_DOCA_COMPRESS */
#endif  /* CONFIG_DOCA */
};

extern int __init docadv_worker_ctx_fetch(struct docadv_worker_context * ctx);
extern int __init docadv_worker_ctx_init(struct docadv_worker_context * ctx);
extern int __init docadv_init(void);

#endif

#endif  /* _DOCA_CONTEXT_H_ */