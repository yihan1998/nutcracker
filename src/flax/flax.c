#include "init.h"
#include "kernel/sched.h"

int flax_worker_ctx_fetch(int core_id) {
    worker_ctx = &contexts[core_id];
#ifdef CONFIG_DOCA
    docadv_worker_ctx_fetch(&worker_ctx->docadv_ctx);
#endif  /* CONFIG_DOCA */
    return 0;
}

int __init flax_init(void) {
    return 0;
}