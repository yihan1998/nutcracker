#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "opt.h"
#include "utils/printk.h"
// #include "lib.h"
#include "kernel/sched.h"

#ifdef CONFIG_DOCA_COMPRESS
#include "drivers/doca/common.h"
#include "drivers/doca/context.h"
#include "drivers/doca/compress.h"

struct docadv_compress_config compress_config = {
	.pci_address = "03:00.0",
};

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

doca_error_t docadv_compress_init(void) {
	doca_error_t result;

	/* Open DOCA device */
	result = open_doca_device_with_pci(compress_config.pci_address, compress_jobs_is_supported, &compress_config.dev);
	if (result != DOCA_SUCCESS) {
#if CONFIG_BLUEFIELD2
		pr_err("No device matching PCI address found. Reason: %s", doca_get_error_string(result));
#else if CONFIG_BLUEFIELD3
		pr_err("No device matching PCI address found. Reason: %s", doca_error_get_descr(result));
#endif
		return result;
	}

	/* Create a DOCA Compress instance */
	result = doca_compress_create(&(compress_config.compress_engine));
	if (result != DOCA_SUCCESS) {
#if CONFIG_BLUEFIELD2
		pr_err("DOCA RegEx creation Failed. Reason: %s", doca_get_error_string(result));
#else if CONFIG_BLUEFIELD3
		pr_err("DOCA RegEx creation Failed. Reason: %s", doca_error_get_descr(result));
#endif
		doca_dev_close(compress_config.dev);
		return DOCA_ERROR_INITIALIZATION;
	}

	/* Set hw Compress device to DOCA Compress */
	result = doca_ctx_dev_add(doca_compress_as_ctx(compress_config.compress_engine), compress_config.dev);
	if (result != DOCA_SUCCESS) {
#if CONFIG_BLUEFIELD2
		pr_err("Unable to install RegEx device. Reason: %s", doca_get_error_string(result));
#else if CONFIG_BLUEFIELD3
		pr_err("Unable to install RegEx device. Reason: %s", doca_error_get_descr(result));
#endif
		result = DOCA_ERROR_INITIALIZATION;
		return 0;
	}

	/* Start DOCA Compress */
    pr_info("Starting DOCA Compress...\n");
	result = doca_ctx_start(doca_compress_as_ctx(compress_config.compress_engine));
	if (result != DOCA_SUCCESS) {
#if CONFIG_BLUEFIELD2
		pr_err("Unable to start DOCA Compress. Reason: %s", doca_get_error_string(result));
#else if CONFIG_BLUEFIELD3
		pr_err("Unable to start DOCA Compress. Reason: %s", doca_error_get_descr(result));
#endif
		result = DOCA_ERROR_INITIALIZATION;
		return 0;
	}

	return DOCA_SUCCESS;
}
#endif  /* CONFIG_DOCA_COMPRESS */