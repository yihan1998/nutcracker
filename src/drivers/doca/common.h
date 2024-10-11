#ifndef _DOCA_COMMON_H_
#define _DOCA_COMMON_H_

#include "percpu.h"
#include "kernel/threads.h"

#ifdef CONFIG_DOCA

#include <doca_buf.h>
#include <doca_buf_inventory.h>
#include <doca_ctx.h>
#include <doca_dev.h>
#include <doca_error.h>
#include <doca_dev.h>

struct buf_info {
    char * data;
    int size;
	struct doca_buf * buf;
	struct doca_mmap * mmap;
};

/* Function to check if a given device is capable of executing some job */
typedef doca_error_t (*jobs_check)(struct doca_devinfo *);

/*
 * Open a DOCA device according to a given PCI address
 *
 * @pci_addr [in]: PCI address
 * @func [in]: pointer to a function that checks if the device have some job capabilities (Ignored if set to NULL)
 * @retval [out]: pointer to doca_dev struct, NULL if not found
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t open_doca_device_with_pci(const char *pci_addr, jobs_check func, struct doca_dev **retval);

doca_error_t init_buf(struct doca_dev * dev, struct doca_buf_inventory * buf_inv, struct buf_info * info, int buf_size);

#endif	/* CONFIG_DOCA */

#endif  /* _DOCA_COMMON_H_ */
