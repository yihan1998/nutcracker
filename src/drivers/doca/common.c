#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <doca_buf.h>
#include <doca_buf_inventory.h>
#include <doca_ctx.h>
#include <doca_dev.h>
#include <doca_error.h>
#include <doca_log.h>
#include <doca_mmap.h>

#include "opt.h"
#include "printk.h"
#include "doca/common.h"
#include "doca/context.h"

doca_error_t open_doca_device_with_pci(const char *pci_addr, jobs_check func, struct doca_dev **retval) {
	struct doca_devinfo **dev_list;
	uint32_t nb_devs;
	uint8_t is_addr_equal = 0;
	int res;
	size_t i;

	/* Set default return value */
	*retval = NULL;

	res = doca_devinfo_list_create(&dev_list, &nb_devs);
	if (res != DOCA_SUCCESS) {
		pr_err("Failed to load doca devices list. Doca_error value: %d\n", res);
		return res;
	}

	/* Search */
	for (i = 0; i < nb_devs; i++) {
		res = doca_devinfo_get_is_pci_addr_equal(dev_list[i], pci_addr, &is_addr_equal);
		if (res == DOCA_SUCCESS && is_addr_equal) {
			/* If any special capabilities are needed */
			if (func != NULL && func(dev_list[i]) != DOCA_SUCCESS) {
				pr_warn("Function not support!\n");
				continue;
			}

			/* if device can be opened */
			res = doca_dev_open(dev_list[i], retval);
			if (res == DOCA_SUCCESS) {
				doca_devinfo_list_destroy(dev_list);
				return res;
			}
		}
	}

	pr_err("Matching device not found\n");
	res = DOCA_ERROR_NOT_FOUND;

	doca_devinfo_list_destroy(dev_list);
	return res;
}

doca_error_t init_buf(struct doca_dev * dev, struct doca_buf_inventory * buf_inv, struct buf_info * info, int buf_size) {
	doca_error_t result;

	info->data = (char *)calloc(buf_size, sizeof(char));
	info->size = buf_size;

	result = doca_mmap_create(NULL, &info->mmap);
	if (result != DOCA_SUCCESS) {
		printf("Unable to create doca_mmap. Reason: %s\n", doca_get_error_string(result));
		return 0;
	}

	result = doca_mmap_dev_add(info->mmap, dev);
	if (result != DOCA_SUCCESS) {
		printf("Unable to add device to doca_mmap. Reason: %s\n", doca_get_error_string(result));
		return 0;
	}

	result = doca_mmap_set_memrange(info->mmap, info->data, info->size);
	if (result != DOCA_SUCCESS) {
		printf("Unable to set memory range of source memory map: %s", doca_get_error_string(result));
		return result;
	}

	result = doca_mmap_start(info->mmap);
	if (result != DOCA_SUCCESS) {
		printf("Unable to start source memory map: %s", doca_get_error_string(result));
		return result;
	}

	if (doca_buf_inventory_buf_by_addr(buf_inv, info->mmap, info->data, info->size, &info->buf) != DOCA_SUCCESS) {
        printf("Failed to create inventory buf!\n");
        return 0;
    }

	return DOCA_SUCCESS;
}