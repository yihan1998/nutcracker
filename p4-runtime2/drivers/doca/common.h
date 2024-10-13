#ifndef _DOCA_COMMON_H_
#define _DOCA_COMMON_H_

#ifdef CONFIG_DOCA

#ifdef __cplusplus
extern "C" {
#endif

#include <doca_buf.h>
#include <doca_buf_inventory.h>
#include <doca_ctx.h>
#include <doca_dev.h>
#include <doca_error.h>
#include <doca_dev.h>
#include <doca_log.h>
#ifdef CONFIG_BLUEFIELD3
#include <doca_pe.h>
#endif

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

#define MAX_PORTS   (2)

extern struct doca_flow_port *ports[MAX_PORTS];
extern struct doca_flow_pipe *rss_pipe[2], *hairpin_pipe[2], *port_pipe[2];

int doca_init();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif	/* CONFIG_DOCA */

#endif  /* _DOCA_COMMON_H_ */