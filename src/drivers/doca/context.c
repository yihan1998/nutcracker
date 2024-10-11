#include <stdio.h>
#include <stdlib.h>

#include "opt.h"
#include "printk.h"
#include "percpu.h"
#include "kernel/threads.h"

#ifdef CONFIG_DOCA
#include "doca/context.h"
#include "doca/compress.h"

// #define MAX_FILE_SIZE 1024

int __init docadv_worker_ctx_fetch(struct docadv_worker_context * ctx) {
#ifdef CONFIG_DOCA_COMPRESS
    doca_error_t res;
	struct docadv_compress_ctx * compress_ctx = &ctx->compress_ctx;

    res = doca_buf_inventory_create(NULL, 2, DOCA_BUF_EXTENSION_NONE, &compress_ctx->buf_inv);
	if (res != DOCA_SUCCESS) {
		printf("Unable to create buffer inventory: %s", doca_get_error_string(res));
		return res;
	}

#if 0
	res = doca_mmap_create(NULL, &compress_ctx->src_buf.mmap);
	if (res != DOCA_SUCCESS) {
		printf("Unable to create source mmap: %s", doca_get_error_string(res));
		return res;
	}
	res = doca_mmap_dev_add(compress_ctx->src_buf.mmap, compress_ctx->dev);
	if (res != DOCA_SUCCESS) {
		printf("Unable to add device to source mmap: %s", doca_get_error_string(res));
		return res;
	}

    res = doca_mmap_create(NULL, &compress_ctx->dst_buf.mmap);
	if (res != DOCA_SUCCESS) {
		printf("Unable to create destination mmap: %s", doca_get_error_string(res));
		return res;
	}
	res = doca_mmap_dev_add(compress_ctx->dst_buf.mmap, compress_ctx->dev);
	if (res != DOCA_SUCCESS) {
		printf("Unable to add device to destination mmap: %s", doca_get_error_string(res));
		return res;
	}

    res = doca_buf_inventory_create(NULL, 2, DOCA_BUF_EXTENSION_NONE, &compress_ctx->buf_inv);
	if (res != DOCA_SUCCESS) {
		printf("Unable to create buffer inventory: %s", doca_get_error_string(res));
		return res;
	}

	res = doca_buf_inventory_start(compress_ctx->buf_inv);
	if (res != DOCA_SUCCESS) {
		printf("Unable to start buffer inventory: %s", doca_get_error_string(res));
		return res;
	}

    compress_ctx->dst_buf.data = calloc(1, MAX_FILE_SIZE);
	if (compress_ctx->dst_buf.data == NULL) {
		printf("Failed to allocate memory");
		return -1;
	}

	res = doca_mmap_set_memrange(compress_ctx->dst_buf.mmap, compress_ctx->dst_buf.data, MAX_FILE_SIZE);
	if (res != DOCA_SUCCESS) {
		return res;
	}
	res = doca_mmap_start(compress_ctx->dst_buf.mmap);
	if (res != DOCA_SUCCESS) {
		free(compress_ctx->dst_buf.data);
		return res;
	}

    compress_ctx->src_buf.data = calloc(1, MAX_FILE_SIZE);
	if (compress_ctx->src_buf.data == NULL) {
		printf("Failed to allocate memory");
		return -1;
	}
	res = doca_mmap_set_memrange(compress_ctx->src_buf.mmap, compress_ctx->src_buf.data, MAX_FILE_SIZE);
	if (res != DOCA_SUCCESS) {
		return res;
	}
	res = doca_mmap_start(compress_ctx->src_buf.mmap);
	if (res != DOCA_SUCCESS) {
		return res;
	}

	/* Construct DOCA buffer for each address range */
	res = doca_buf_inventory_buf_by_addr(compress_ctx->buf_inv, compress_ctx->src_buf.mmap, compress_ctx->src_buf.data, strlen(compress_ctx->src_buf.data), &compress_ctx->src_buf.buf);
	if (res != DOCA_SUCCESS) {
		printf("Unable to acquire DOCA buffer representing source buffer: %s", doca_get_error_string(res));
		return res;
	}

	/* Construct DOCA buffer for each address range */
	res = doca_buf_inventory_buf_by_addr(compress_ctx->buf_inv, compress_ctx->dst_buf.mmap, compress_ctx->dst_buf.data, MAX_FILE_SIZE, &compress_ctx->dst_buf.buf);
	if (res != DOCA_SUCCESS) {
		printf("Unable to acquire DOCA buffer representing destination buffer: %s", doca_get_error_string(res));
		return res;
	}
#endif
    init_buf(compress_ctx->dev, compress_ctx->buf_inv, &compress_ctx->src_buf, 32768);
    init_buf(compress_ctx->dev, compress_ctx->buf_inv, &compress_ctx->dst_buf, 32768);
#endif  /* CONFIG_DOCA_COMPRESS */

    return DOCA_SUCCESS;
}

int __init docadv_worker_ctx_init(struct docadv_worker_context * ctx) {
    doca_error_t result;

    result = doca_workq_create(WORKQ_DEPTH, &ctx->workq);
	if (result != DOCA_SUCCESS) {
		printf("Unable to create work queue. Reason: %s", doca_get_error_string(result));
		return result;
	}
#ifdef CONFIG_DOCA_SHA
	ctx->sha_ctx.dev = doca_sha_cfg.dev;
	ctx->sha_ctx.doca_sha = doca_sha_cfg.doca_sha;

    /* Add workq to RegEx */
	result = doca_ctx_workq_add(doca_sha_as_ctx(ctx->sha_ctx.doca_sha), ctx->workq);
	if (result != DOCA_SUCCESS) {
		printf("Unable to attach work queue to SHA. Reason: %s\n", doca_get_error_string(result));
		return result;
	}
#endif  /* CONFIG_DOCA_SHA */
#ifdef CONFIG_DOCA_COMPRESS
	ctx->compress_ctx.dev = compress_config.dev;
	ctx->compress_ctx.compress_engine = compress_config.compress_engine;

    /* Add workq to RegEx */
	result = doca_ctx_workq_add(doca_compress_as_ctx(ctx->compress_ctx.compress_engine), ctx->workq);
	if (result != DOCA_SUCCESS) {
		printf("Unable to attach work queue to Compress. Reason: %s\n", doca_get_error_string(result));
		return result;
	}
#endif  /* CONFIG_DOCA_COMPRESS */
    return DOCA_SUCCESS;
}

int __init docadv_init(void) {
#ifdef CONFIG_DOCA_COMPRESS
	pr_info("Init: DOCA device...\n");
	docadv_compress_init();
#endif /* CONFIG_DOCA_COMPRESS */
    return 0;
}

#endif /* CONFIG_DOCA */