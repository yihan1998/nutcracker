#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <dlfcn.h>
#include <link.h>

#include "opt.h"
#include "printk.h"
#include "interpose.h"
#include "kernel/sched.h"
#include "net/dpdk_module.h"
#ifdef CONFIG_DOCA
#include "doca/context.h"
#endif  /* CONFIG_DOCA */

bool kernel_early_boot = true;

/* Original main() entry and parameters */
int (*main_orig)(int, char **, char **);
int main_argc;
char ** main_argv;
char ** main_envp;

struct worker_context context;

#define MAX_FILE_SIZE   1024

/* DOCA core objects used by the samples / applications */
struct program_core_objects {
	struct doca_dev *dev;			/* doca device */
	struct doca_mmap *src_mmap;		/* doca mmap for source buffer */
	struct doca_mmap *dst_mmap;		/* doca mmap for destination buffer */
	struct doca_buf_inventory *buf_inv;	/* doca buffer inventory */
	struct doca_ctx *ctx;			/* doca context */
	struct doca_workq *workq;		/* doca work queue */
};

doca_error_t
create_core_objects(struct program_core_objects *state, uint32_t max_bufs)
{
	doca_error_t res;

	res = doca_mmap_create(NULL, &state->src_mmap);
	if (res != DOCA_SUCCESS) {
		printf("Unable to create source mmap: %s", doca_get_error_string(res));
		return res;
	}
	res = doca_mmap_dev_add(state->src_mmap, state->dev);
	if (res != DOCA_SUCCESS) {
		printf("Unable to add device to source mmap: %s", doca_get_error_string(res));
		doca_mmap_destroy(state->src_mmap);
		state->src_mmap = NULL;
		return res;
	}

	res = doca_mmap_create(NULL, &state->dst_mmap);
	if (res != DOCA_SUCCESS) {
		printf("Unable to create destination mmap: %s", doca_get_error_string(res));
		return res;
	}
	res = doca_mmap_dev_add(state->dst_mmap, state->dev);
	if (res != DOCA_SUCCESS) {
		printf("Unable to add device to destination mmap: %s", doca_get_error_string(res));
		doca_mmap_destroy(state->dst_mmap);
		state->dst_mmap = NULL;
		return res;
	}

	res = doca_buf_inventory_create(NULL, max_bufs, DOCA_BUF_EXTENSION_NONE, &state->buf_inv);
	if (res != DOCA_SUCCESS) {
		printf("Unable to create buffer inventory: %s", doca_get_error_string(res));
		return res;
	}

	res = doca_buf_inventory_start(state->buf_inv);
	if (res != DOCA_SUCCESS) {
		printf("Unable to start buffer inventory: %s", doca_get_error_string(res));
		return res;
	}

	res = doca_ctx_dev_add(state->ctx, state->dev);
	if (res != DOCA_SUCCESS) {
		printf("Unable to register device with lib context: %s", doca_get_error_string(res));
		state->ctx = NULL;
		return res;
	}

	return res;
}

doca_error_t
start_context(struct program_core_objects *state, uint32_t workq_depth)
{
	doca_error_t res;
	struct doca_workq *workq;

	res = doca_ctx_start(state->ctx);
	if (res != DOCA_SUCCESS) {
		printf("Unable to start lib context: %s", doca_get_error_string(res));
		doca_ctx_dev_rm(state->ctx, state->dev);
		state->ctx = NULL;
		return res;
	}

	res = doca_workq_create(workq_depth, &workq);
	if (res != DOCA_SUCCESS) {
		printf("Unable to create work queue: %s", doca_get_error_string(res));
		return res;
	}

	res = doca_ctx_workq_add(state->ctx, workq);
	if (res != DOCA_SUCCESS) {
		printf("Unable to register work queue with context: %s", doca_get_error_string(res));
		state->workq = NULL;
	} else
		state->workq = workq;

	return res;
}

doca_error_t
init_core_objects(struct program_core_objects *state, uint32_t workq_depth, uint32_t max_bufs)
{
	doca_error_t res;

	res = create_core_objects(state, max_bufs);
	if (res != DOCA_SUCCESS)
		return res;
	res = start_context(state, workq_depth);
	return res;
}

doca_error_t
compress_jobs_decompress_is_supported(struct doca_devinfo *devinfo)
{
	return doca_compress_job_get_supported(devinfo, DOCA_COMPRESS_DEFLATE_JOB);
}

doca_error_t compress_deflate2(char *file_data, size_t file_size, char * out) {
    doca_error_t res;
	struct docadv_compress_ctx * compress_ctx = &context.docadv_ctx.compress_ctx;
	struct doca_event event = {0};
    void * mbuf_data;
    uint8_t * resp_head;
    size_t resp_len;

    // printf("[%s:%d] data -> len: %lu\n", __func__, __LINE__, file_size);
    // for (int i = 0; i < file_size; i++) {
    //     printf("%02x", file_data[i]);
    // }
    // printf("\n");

    memcpy(compress_ctx->src_buf.data, file_data, file_size);
    doca_buf_get_data(compress_ctx->src_buf.buf, &mbuf_data);
    doca_buf_set_data(compress_ctx->src_buf.buf, mbuf_data, file_size);

	/* Construct compress job */
	const struct doca_compress_deflate_job compress_job = {
		.base = (struct doca_job) {
			.type = DOCA_COMPRESS_DEFLATE_JOB,
			.flags = DOCA_JOB_FLAGS_NONE,
			.ctx = doca_compress_as_ctx(compress_ctx->compress_engine),
			.user_data.u64 = DOCA_COMPRESS_DEFLATE_JOB,
			},
		.dst_buff = compress_ctx->dst_buf.buf,
		.src_buff = compress_ctx->src_buf.buf,
	};

	/* Enqueue compress job */
	res = doca_workq_submit(context.docadv_ctx.workq, &compress_job.base);
	if (res != DOCA_SUCCESS) {
		printf("Failed to submit compress job: %s\n", doca_get_error_string(res));
		doca_buf_refcount_rm(compress_ctx->dst_buf.buf, NULL);
		doca_buf_refcount_rm(compress_ctx->src_buf.buf, NULL);
		return res;
	}

    do {
		res = doca_workq_progress_retrieve(context.docadv_ctx.workq, &event, DOCA_WORKQ_RETRIEVE_FLAGS_NONE);
	    if (res != DOCA_SUCCESS) {
            if (res == DOCA_ERROR_AGAIN) {
                continue;
            } else {
                printf("Unable to dequeue results. [%s]\n", doca_get_error_string(res));
            }
        }
	} while (res != DOCA_SUCCESS);

    doca_buf_get_data(compress_ctx->dst_buf.buf, (void **)&resp_head);
    doca_buf_get_data_len(compress_ctx->dst_buf.buf, &resp_len);

    // printf("\tDOCA compressed -> len: %ld\n", resp_len);
    // for (int i = 0; i < resp_len; i++) {
    //     printf("%02x", resp_head[i]);
    // }
    // printf("\n");

    memcpy(out, resp_head, resp_len);

    doca_buf_reset_data_len(compress_ctx->dst_buf.buf);

    return resp_len;
}

doca_error_t compress_inflate2(char *file_data, size_t file_size) {
    doca_error_t res;
	struct docadv_compress_ctx * compress_ctx = &context.docadv_ctx.compress_ctx;
	struct doca_event event = {0};
    void * mbuf_data;
    uint8_t * resp_head;
    size_t resp_len;

    printf("[%s:%d] data -> len: %lu\n", __func__, __LINE__, file_size);
    for (int i = 0; i < file_size; i++) {
        printf("%02x", file_data[i]);
    }
    printf("\n");

    memcpy(compress_ctx->src_buf.data, file_data, file_size);
    doca_buf_get_data(compress_ctx->src_buf.buf, &mbuf_data);
    doca_buf_set_data(compress_ctx->src_buf.buf, mbuf_data, file_size);

	/* Construct compress job */
	const struct doca_compress_deflate_job compress_job = {
		.base = (struct doca_job) {
			.type = DOCA_DECOMPRESS_DEFLATE_JOB,
			.flags = DOCA_JOB_FLAGS_NONE,
			.ctx = doca_compress_as_ctx(compress_ctx->compress_engine),
			.user_data.u64 = DOCA_COMPRESS_DEFLATE_JOB,
			},
		.dst_buff = compress_ctx->dst_buf.buf,
		.src_buff = compress_ctx->src_buf.buf,
	};

	/* Enqueue compress job */
	res = doca_workq_submit(context.docadv_ctx.workq, &compress_job.base);
	if (res != DOCA_SUCCESS) {
		printf("Failed to submit compress job: %s\n", doca_get_error_string(res));
		doca_buf_refcount_rm(compress_ctx->dst_buf.buf, NULL);
		doca_buf_refcount_rm(compress_ctx->src_buf.buf, NULL);
		return res;
	}

    do {
		res = doca_workq_progress_retrieve(context.docadv_ctx.workq, &event, DOCA_WORKQ_RETRIEVE_FLAGS_NONE);
	    if (res != DOCA_SUCCESS) {
            if (res == DOCA_ERROR_AGAIN) {
                continue;
            } else {
                printf("Unable to dequeue results. [%s]\n", doca_get_error_string(res));
            }
        }
	} while (res != DOCA_SUCCESS);

    doca_buf_get_data(compress_ctx->dst_buf.buf, (void **)&resp_head);
    doca_buf_get_data_len(compress_ctx->dst_buf.buf, &resp_len);

    printf("\tDOCA decompressed -> len: %ld\n", resp_len);
    for (int i = 0; i < resp_len; i++) {
        printf("%02x", resp_head[i]);
    }
    printf("\n");

    doca_buf_reset_data_len(compress_ctx->dst_buf.buf);

    return DOCA_SUCCESS;
}

// char msg[] = "helloworld";
// char out[64];

/* Our fake main() that gets called by __libc_start_main() */
int main_hook(int argc, char ** argv, char ** envp) {
	/* Get which CPU we are spawned on */
	cpu_id = sched_getcpu();

	pr_info("INIT: starting Cygnus...\n");

	pr_info("INIT: kernel initialization phase\n");

	main_argc = argc;
	main_argv = argv;
	main_envp = envp;

	char * args[] = {
		"", "-c1", "-n4", "-a03:00.1", "--proc-type=secondary"
	};

    pr_info("INIT: initialize DPDK...\n");
    dpdk_init(5, args);

#ifdef CONFIG_DOCA
    pr_info("INIT: initialize DOCA...\n");
    docadv_init();
#endif  /* CONFIG_DOCA */

#ifdef CONFIG_DOCA
    // struct worker_context * ctx = &contexts[1];
    // docadv_worker_ctx_init(&ctx->docadv_ctx);
    // docadv_worker_ctx_fetch(&ctx->docadv_ctx);
    // worker_ctx = &contexts[1];
    docadv_worker_ctx_init(&context.docadv_ctx);
    docadv_worker_ctx_fetch(&context.docadv_ctx);
    worker_ctx = &context;
#endif  /* CONFIG_DOCA */

    pr_info("init: initializing zlib...\n");
    zlib_hook_init();

    kernel_early_boot = false;

    cpu_set_t set;
    CPU_ZERO(&set);
    // Set CPU 0 and CPU 1
    CPU_SET(1, &set); // Allow execution on CPU 1

    // Set the CPU affinity for the current process
    if (sched_setaffinity(getpid(), sizeof(cpu_set_t), &set) == -1) {
        perror("sched_setaffinity");
        exit(EXIT_FAILURE);
    }

    pr_info("INIT: initialize fs...\n");
    fs_init();

    pr_info("INIT: initialize net...\n");
    net_init();

    // int len = compress_deflate2(msg, strlen(msg), out);

    // compress_inflate2(out, len);

    return main_orig(main_argc, main_argv, main_envp);
}

/**
 * 	Wrapper for __libc_start_main() that replaces the real main function 
 * 	@param main - original main function
 * 	@param init	- initialization for main
 */
int __libc_start_main(	int (*main)(int, char **, char **), int argc, char **argv, 
						int (*init)(int, char **, char **), void (*fini)(void), void (*rtld_fini)(void), void * stack_end) {
    /* Save the real main function address */
    main_orig = main;

    /* Find the real __libc_start_main()... */
    typeof(&__libc_start_main) orig = dlsym(RTLD_NEXT, "__libc_start_main");

    /* ... and call it with custom main function */
    return orig(main_hook, argc, argv, init, fini, rtld_fini, stack_end);
}