#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "opt.h"
#include "printk.h"
#include "lib.h"
#include "kernel/sched.h"

#ifdef CONFIG_DOCA_COMPRESS
#include "doca/common.h"
#include "doca/context.h"
#include "doca/compress.h"

struct docadv_compress_config compress_config = {
	.pci_address = "03:00.0",
};

int docadv_deflate(z_streamp strm, int flush) {
    doca_error_t res;
	struct docadv_compress_ctx * compress_ctx = &worker_ctx->docadv_ctx.compress_ctx;
	struct doca_event event = {0};
    void * mbuf_data;
    uint8_t * resp_head;
    size_t resp_len;

    // printf("[%s:%d] data -> len: %d\n", __func__, __LINE__, strm->avail_in);
    // for (int i = 0; i < strm->avail_in; i++) {
    //     printf("%02x", strm->next_in[i]);
    // }
    // printf("\n");

    memcpy(compress_ctx->src_buf.data, strm->next_in, strm->avail_in);
    doca_buf_get_data(compress_ctx->src_buf.buf, &mbuf_data);
    doca_buf_set_data(compress_ctx->src_buf.buf, mbuf_data, strm->avail_in);

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
	res = doca_workq_submit(worker_ctx->docadv_ctx.workq, &compress_job.base);
	if (res != DOCA_SUCCESS) {
		printf("Failed to submit compress job: %s\n", doca_get_error_string(res));
		doca_buf_refcount_rm(compress_ctx->dst_buf.buf, NULL);
		doca_buf_refcount_rm(compress_ctx->src_buf.buf, NULL);
		return res;
	}

    do {
		res = doca_workq_progress_retrieve(worker_ctx->docadv_ctx.workq, &event, DOCA_WORKQ_RETRIEVE_FLAGS_NONE);
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

    memcpy(strm->next_out, resp_head, resp_len);
    strm->avail_out -= resp_len;

    doca_buf_reset_data_len(compress_ctx->dst_buf.buf);

    return DOCA_SUCCESS;
}

int docadv_deflateInit_(z_streamp strm, int level, const char *version, int stream_size) {
    return 0;
}

int docadv_deflateEnd(z_streamp strm) {
    return 0;
}

int docadv_inflate(z_streamp strm, int flush) {
    doca_error_t res;
	struct docadv_compress_ctx * compress_ctx = &worker_ctx->docadv_ctx.compress_ctx;
	struct doca_event event = {0};
    void * mbuf_data;
    uint8_t * resp_head;
    size_t resp_len;

    // fprintf(stderr, "data -> len: %d\n", strm->avail_in);
    // for (int i = 0; i < strm->avail_in; i++) {
    //     fprintf(stderr, "%02x", strm->next_in[i]);
    // }
    // fprintf(stderr, "\n");

    memcpy(compress_ctx->src_buf.data, strm->next_in, strm->avail_in);
    doca_buf_get_data(compress_ctx->src_buf.buf, &mbuf_data);
    doca_buf_set_data(compress_ctx->src_buf.buf, mbuf_data, strm->avail_in);
    // printf("Data len: %d, %*s\n", strm->avail_in, strm->avail_in, strm->next_in);

	/* Construct compress job */
	const struct doca_compress_deflate_job compress_job = {
		.base = (struct doca_job) {
			.type = DOCA_DECOMPRESS_DEFLATE_JOB,
			.flags = DOCA_JOB_FLAGS_NONE,
			.ctx = doca_compress_as_ctx(compress_ctx->compress_engine),
			.user_data.u64 = DOCA_DECOMPRESS_DEFLATE_JOB,
			},
		.dst_buff = compress_ctx->dst_buf.buf,
		.src_buff = compress_ctx->src_buf.buf,
	};

	/* Enqueue compress job */
	res = doca_workq_submit(worker_ctx->docadv_ctx.workq, &compress_job.base);
	if (res != DOCA_SUCCESS) {
		printf("Failed to submit decompress job: %s\n", doca_get_error_string(res));
		doca_buf_refcount_rm(compress_ctx->dst_buf.buf, NULL);
		doca_buf_refcount_rm(compress_ctx->src_buf.buf, NULL);
		return res;
	}

    do {
		res = doca_workq_progress_retrieve(worker_ctx->docadv_ctx.workq, &event, DOCA_WORKQ_RETRIEVE_FLAGS_NONE);
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

    // printf("\tDOCA decompressed -> len: %ld\n", resp_len);
    // for (int i = 0; i < resp_len; i++) {
    //     printf("%02x", resp_head[i]);
    // }
    // printf("\n");

    memcpy(strm->next_out, resp_head, resp_len);
    strm->avail_out -= resp_len;

    doca_buf_reset_data_len(compress_ctx->dst_buf.buf);

    return DOCA_SUCCESS;
}

int docadv_inflateInit_(z_streamp strm, const char *version, int stream_size) {
    return 0;
}

int docadv_inflateEnd(z_streamp strm) {
    return 0;
}

/**
 * Check if given device is capable of executing a DOCA_COMPRESS_DEFLATE_JOB.
 *
 * @devinfo [in]: The DOCA device information
 * @return: DOCA_SUCCESS if the device supports DOCA_COMPRESS_DEFLATE_JOB and DOCA_ERROR otherwise.
 */
static doca_error_t
compress_jobs_is_supported(struct doca_devinfo *devinfo)
{
    if (doca_compress_job_get_supported(devinfo, DOCA_COMPRESS_DEFLATE_JOB) == DOCA_SUCCESS) {
        if (doca_compress_job_get_supported(devinfo, DOCA_DECOMPRESS_DEFLATE_JOB) == DOCA_SUCCESS) {
            return DOCA_SUCCESS;
        } else {
            pr_err("DECOMPRESS unsupported!\n");
            return DOCA_ERROR_NOT_SUPPORTED;
        }
    }

    pr_err("COMPRESS unsupported!\n");
	return DOCA_ERROR_NOT_SUPPORTED;
}

/*
 * RegEx context initialization
 *
 * @doca_regex_cfg [in/out]: application configuration structure
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t docadv_compress_init(void) {
#ifdef CONFIG_DOCA_COMPRESS
	doca_error_t result;

	/* Open DOCA device */
	result = open_doca_device_with_pci(compress_config.pci_address, compress_jobs_is_supported, &compress_config.dev);
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

#endif /* CONFIG_DOCA_COMPRESS */