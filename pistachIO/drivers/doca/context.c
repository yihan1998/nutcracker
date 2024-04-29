#include "opt.h"
#include "printk.h"
#include "kernel/threads.h"
#include "doca/context.h"
#include "doca/regex.h"

#define WORKQ_DEPTH	128

DEFINE_PER_CPU(struct worker_context *, worker_ctx);

int __init doca_percore_init(struct worker_context * ctx) {
#ifdef CONFIG_DOCA_REGEX
	doca_regex_percore_init(&ctx->regex_ctx);
#endif	/* CONFIG_DOCA_REGEX */
    return 0;
}

int __init doca_worker_init(struct worker_context * ctx) {
#if CONFIG_DOCA
	doca_error_t result;

    result = doca_workq_create(WORKQ_DEPTH, &ctx->workq);
	if (result != DOCA_SUCCESS) {
		pr_err("Unable to create work queue. Reason: %s", doca_get_error_string(result));
		return result;
	}
#ifdef CONFIG_DOCA_REGEX
	ctx->regex_ctx.dev = doca_regex_cfg.dev;
	ctx->regex_ctx.doca_reg = doca_regex_cfg.doca_reg;

    /* Add workq to RegEx */
	result = doca_ctx_workq_add(doca_regex_as_ctx(ctx->regex_ctx.doca_reg), ctx->workq);
	if (result != DOCA_SUCCESS) {
		pr_err("Unable to attach work queue to REGEX. Reason: %s", doca_get_error_string(result));
		return result;
	}
#endif
#endif
    return DOCA_SUCCESS;
}

int __init doca_init(void) {
#ifdef CONFIG_DOCA_REGEX
	doca_regex_init();
#endif	/* CONFIG_DOCA_REGEX */
	return 0;
}