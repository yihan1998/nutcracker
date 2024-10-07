#ifndef _DOCA_CONTEXT_H_
#define _DOCA_CONTEXT_H_

#include "init.h"
#include "percpu.h"

#ifdef CONFIG_DOCA

#ifdef __cplusplus
extern "C" {
#endif

#include "drivers/doca/compress.h"

#define WORKQ_DEPTH	128

struct docadv_context {
#ifdef CONFIG_DOCA
#ifdef CONFIG_BLUEFIELD2
	struct doca_workq * workq;   /* DOCA work queue */
#elif CONFIG_BLUEFIELD3
	struct doca_pe  * pe;		    /* doca progress engine */
#endif
#ifdef CONFIG_DOCA_REGEX
    struct regex_ctx regex_ctx;
#endif  /* CONFIG_DOCA_REGEX */
#ifdef CONFIG_DOCA_COMPRESS
    struct docadv_compress_ctx compress_ctx;
#endif  /* CONFIG_DOCA_COMPRESS */
#endif  /* CONFIG_DOCA */
};

extern int __init docadv_ctx_fetch(struct docadv_context * ctx);
extern int __init docadv_ctx_init(struct docadv_context * ctx);
extern int __init docadv_init(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

#endif  /* _DOCA_CONTEXT_H_ */