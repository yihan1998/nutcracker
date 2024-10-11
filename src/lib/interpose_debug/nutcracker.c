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
#include "percpu.h"
#include "kernel/sched.h"
#include "net/dpdk_module.h"
#ifdef CONFIG_DOCA
#include "doca/context.h"
#endif  /* CONFIG_DOCA */

bool kernel_early_boot = true;

struct worker_context context;

#define CHUNK 1024

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

static doca_error_t
compress_jobs_decompress_is_supported(struct doca_devinfo *devinfo)
{
	return doca_compress_job_get_supported(devinfo, DOCA_COMPRESS_DEFLATE_JOB);
}

doca_error_t compress_deflate(const char *pci_addr, char *file_data, size_t file_size, enum doca_compress_job_types job_type) {
	struct program_core_objects state = {0};
	struct doca_event event = {0};
	struct doca_compress *compress;
	struct doca_buf *src_doca_buf;
	struct doca_buf *dst_doca_buf;
	uint32_t workq_depth = 1;		/* The sample will run 1 compress job */
	uint32_t max_bufs = 2;			/* The sample will use 2 doca buffers */
	char *dst_buffer;
	doca_error_t result;

	result = doca_compress_create(&compress);
	if (result != DOCA_SUCCESS) {
		printf("Unable to create compress engine: %s", doca_get_error_string(result));
		return result;
	}

	state.ctx = doca_compress_as_ctx(compress);

    result = open_doca_device_with_pci(pci_addr, compress_jobs_decompress_is_supported, &state.dev);

	if (result != DOCA_SUCCESS) {
		doca_compress_destroy(compress);
		return result;
	}

	result = init_core_objects(&state, workq_depth, max_bufs);
	if (result != DOCA_SUCCESS) {
		return result;
	}

	dst_buffer = calloc(1, MAX_FILE_SIZE);
	if (dst_buffer == NULL) {
		printf("Failed to allocate memory");
		return -1;
	}

	result = doca_mmap_set_memrange(state.dst_mmap, dst_buffer, MAX_FILE_SIZE);
	if (result != DOCA_SUCCESS) {
		return result;
	}
	result = doca_mmap_start(state.dst_mmap);
	if (result != DOCA_SUCCESS) {
		free(dst_buffer);
		return result;
	}

	result = doca_mmap_set_memrange(state.src_mmap, file_data, file_size);
	if (result != DOCA_SUCCESS) {
		return result;
	}
	result = doca_mmap_start(state.src_mmap);
	if (result != DOCA_SUCCESS) {
		return result;
	}

	/* Construct DOCA buffer for each address range */
	result = doca_buf_inventory_buf_by_addr(state.buf_inv, state.src_mmap, file_data, file_size, &src_doca_buf);
	if (result != DOCA_SUCCESS) {
		printf("Unable to acquire DOCA buffer representing source buffer: %s", doca_get_error_string(result));
		return result;
	}

	/* Construct DOCA buffer for each address range */
	result = doca_buf_inventory_buf_by_addr(state.buf_inv, state.dst_mmap, dst_buffer, MAX_FILE_SIZE, &dst_doca_buf);
	if (result != DOCA_SUCCESS) {
		printf("Unable to acquire DOCA buffer representing destination buffer: %s", doca_get_error_string(result));
		return result;
	}

	/* setting data length in doca buffer */
	result = doca_buf_set_data(src_doca_buf, file_data, file_size);
	if (result != DOCA_SUCCESS) {
		printf("Unable to set DOCA buffer data: %s\n", doca_get_error_string(result));
		doca_buf_refcount_rm(src_doca_buf, NULL);
		doca_buf_refcount_rm(dst_doca_buf, NULL);
		return result;
	}

	/* Construct compress job */
	const struct doca_compress_deflate_job compress_job = {
		.base = (struct doca_job) {
			.type = job_type,
			.flags = DOCA_JOB_FLAGS_NONE,
			.ctx = state.ctx,
			.user_data.u64 = job_type,
			},
		.dst_buff = dst_doca_buf,
		.src_buff = src_doca_buf,
	};

	/* Enqueue compress job */
	result = doca_workq_submit(state.workq, &compress_job.base);
	if (result != DOCA_SUCCESS) {
		printf("Failed to submit compress job: %s\n", doca_get_error_string(result));
		doca_buf_refcount_rm(dst_doca_buf, NULL);
		doca_buf_refcount_rm(src_doca_buf, NULL);
		return result;
	}

	/* Wait for job completion */
	while ((result = doca_workq_progress_retrieve(state.workq, &event, DOCA_WORKQ_RETRIEVE_FLAGS_NONE)) == DOCA_ERROR_AGAIN);

	if (result != DOCA_SUCCESS)
		printf("Failed to retrieve compress job: %s\n", doca_get_error_string(result));

	else if (event.result.u64 != DOCA_SUCCESS)
		printf("Compress job finished unsuccessfully\n");

	else if (((int)(event.type) != (int)job_type) || (event.user_data.u64 != job_type))
		printf("Received wrong event\n");

	else {
		printf("Compress done!\n");
	}

	if (doca_buf_refcount_rm(src_doca_buf, NULL) != DOCA_SUCCESS ||
	    doca_buf_refcount_rm(dst_doca_buf, NULL) != DOCA_SUCCESS)
		printf("Failed to decrease DOCA buffer reference count\n");

	return result;
}

doca_error_t compress_deflate2(char *file_data, size_t file_size, char * out) {
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

    printf("\tDOCA compressed -> len: %ld\n", resp_len);
    for (int i = 0; i < resp_len; i++) {
        printf("%02x", resp_head[i]);
    }
    printf("\n");

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


// Compress data using DEFLATE algorithm
unsigned char* compress_data(const unsigned char *data, size_t data_len, size_t *compressed_len) {
    z_stream strm;
    unsigned char *compressed_data = NULL;
    size_t chunk_size = CHUNK;

    // Initialize zlib stream for compression
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    if (deflateInit(&strm, Z_DEFAULT_COMPRESSION) != Z_OK) {
        fprintf(stderr, "deflateInit failed\n");
        return NULL;
    }

    // Allocate buffer for compressed data
    compressed_data = (unsigned char *)malloc(chunk_size);
    if (!compressed_data) {
        fprintf(stderr, "Memory allocation failed\n");
        deflateEnd(&strm);
        return NULL;
    }

    strm.avail_in = data_len;
    strm.next_in = (unsigned char *)data;
    strm.avail_out = chunk_size;
    strm.next_out = compressed_data;

    // Compress the data
    if (deflate(&strm, Z_FINISH) != Z_STREAM_END) {
        fprintf(stderr, "deflate failed\n");
        free(compressed_data);
        deflateEnd(&strm);
        return NULL;
    }

    *compressed_len = chunk_size - strm.avail_out;
    deflateEnd(&strm);
    return compressed_data;
}

// Decompress data using DEFLATE algorithm
unsigned char* decompress_data(const unsigned char *compressed_data, size_t compressed_len, size_t *decompressed_len) {
    z_stream strm;
    unsigned char *decompressed_data = NULL;
    size_t chunk_size = CHUNK;

    // Initialize zlib stream for decompression
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    if (inflateInit(&strm) != Z_OK) {
        fprintf(stderr, "inflateInit failed\n");
        return NULL;
    }

    // Allocate buffer for decompressed data
    decompressed_data = (unsigned char *)malloc(chunk_size);
    if (!decompressed_data) {
        fprintf(stderr, "Memory allocation failed\n");
        inflateEnd(&strm);
        return NULL;
    }

    strm.avail_in = compressed_len;
    strm.next_in = (unsigned char *)compressed_data;
    strm.avail_out = chunk_size;
    strm.next_out = decompressed_data;

    // Decompress the data
    if (inflate(&strm, Z_NO_FLUSH) != Z_STREAM_END) {
        fprintf(stderr, "inflate failed\n");
        free(decompressed_data);
        inflateEnd(&strm);
        return NULL;
    }

    *decompressed_len = chunk_size - strm.avail_out;
    inflateEnd(&strm);
    return decompressed_data;
}

static doca_error_t
compress_jobs_compress_is_supported(struct doca_devinfo *devinfo)
{
	return doca_compress_job_get_supported(devinfo, DOCA_COMPRESS_DEFLATE_JOB);
}

doca_error_t _docadv_compress_init(void) {
#ifdef CONFIG_DOCA_COMPRESS
	doca_error_t result;

	/* Open DOCA device */
	result = open_doca_device_with_pci(compress_config.pci_address, compress_jobs_compress_is_supported, &compress_config.dev);
	if (result != DOCA_SUCCESS) {
		pr_err("No device matching PCI address found. Reason: %s", doca_get_error_string(result));
		return result;
	}

	/* Create a DOCA Compress instance */
	result = doca_compress_create(&(compress_config.compress_engine));
	if (result != DOCA_SUCCESS) {
		pr_err("DOCA RegEx creation Failed. Reason: %s", doca_get_error_string(result));
		doca_dev_close(compress_config.dev);
		return DOCA_ERROR_INITIALIZATION;
	}

	/* Set hw Compress device to DOCA Compress */
	result = doca_ctx_dev_add(doca_compress_as_ctx(compress_config.compress_engine), compress_config.dev);
	if (result != DOCA_SUCCESS) {
		pr_err("Unable to install RegEx device. Reason: %s", doca_get_error_string(result));
		result = DOCA_ERROR_INITIALIZATION;
		return 0;
	}

	/* Start DOCA Compress */
    pr_info("Starting DOCA Compress...\n");
	result = doca_ctx_start(doca_compress_as_ctx(compress_config.compress_engine));
	if (result != DOCA_SUCCESS) {
		pr_err("Unable to start DOCA Compress. Reason: %s", doca_get_error_string(result));
		result = DOCA_ERROR_INITIALIZATION;
		return 0;
	}
#endif  /* CONFIG_DOCA_COMPRESS */
	return DOCA_SUCCESS;
}

int main(int argc, char ** argv) {
// 	/* Get which CPU we are spawned on */
// 	cpu_id = sched_getcpu();

// 	pr_info("INIT: starting Cygnus...\n");

// 	pr_info("INIT: kernel initialization phase\n");

// 	char * args[] = {
// 		"", "-c1", "-n4", "-a03:00.1", "--proc-type=secondary"
// 	};

//     pr_info("INIT: initialize DPDK...\n");
//     dpdk_init(5, args);

#ifdef CONFIG_DOCA
    pr_info("INIT: initialize DOCA...\n");
    _docadv_compress_init();
#endif  /* CONFIG_DOCA */

    // kernel_early_boot = false;

//     cpu_set_t set;
//     CPU_ZERO(&set);
//     // Set CPU 0 and CPU 1
//     CPU_SET(1, &set); // Allow execution on CPU 1

//     // Set the CPU affinity for the current process
//     if (sched_setaffinity(getpid(), sizeof(cpu_set_t), &set) == -1) {
//         perror("sched_setaffinity");
//         exit(EXIT_FAILURE);
//     }

//     pr_info("INIT: initialize fs...\n");
//     fs_init();

//     pr_info("INIT: initialize net...\n");
//     net_init();

    pr_info("init: initializing zlib...\n");
    zlib_hook_init();

#ifdef CONFIG_DOCA
    docadv_worker_ctx_init(&context.docadv_ctx);
    docadv_worker_ctx_fetch(&context.docadv_ctx);
    worker_ctx = &context;
#endif  /* CONFIG_DOCA */

    // char msg[] = "helloworld";
    // char out[64];
    // int len = compress_deflate2(msg, strlen(msg), out);

    // compress_inflate2(out, len);

    const unsigned char original_data[] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    size_t original_len = strlen((const char *)original_data);
    size_t compressed_len;
    size_t decompressed_len;

    // Compress the data
    unsigned char *compressed_data = compress_data(original_data, original_len, &compressed_len);
    if (!compressed_data) {
        fprintf(stderr, "Compression failed\n");
        return 1;
    }
    printf("Compressed Data: ");
    for (size_t i = 0; i < compressed_len; ++i) {
        printf("%02x ", compressed_data[i]);
    }
    printf("\n");

    // Decompress the data
    unsigned char *decompressed_data = decompress_data(compressed_data, compressed_len, &decompressed_len);
    if (!decompressed_data) {
        fprintf(stderr, "Decompression failed\n");
        free(compressed_data);
        return 1;
    }
    printf("Decompressed Data: %.*s\n", (int)decompressed_len, decompressed_data);

    // Clean up
    free(compressed_data);
    free(decompressed_data);

    return 0;
}