#ifndef _TEST_DOCA_REGEX_H_
#define _TEST_DOCA_REGEX_H_

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <doca_buf.h>
#include <doca_buf_inventory.h>
#include <doca_dev.h>
#include <doca_mmap.h>
#include <doca_regex.h>
#include <doca_regex_mempool.h>

#define MAX_FILE_NAME 255		/* Maximal length of file path */
#define BUF_INVENTORY_POOL_SIZE 1000	/* Number of elements in the buffer inventory */

struct test_dns_config {
	char rules_file_path[MAX_FILE_NAME];		/* Path to RegEx rules file */
	char pci_address[DOCA_DEVINFO_PCI_ADDR_SIZE];	/* RegEx PCI address to use */
	char *data_buffer;				/* Data buffer */
	size_t data_buffer_len;				/* Data buffer length */
	char *rules_buffer;				/* RegEx rules buffer */
	size_t rules_buffer_len;			/* RegEx rules buffer length */

	struct doca_buf_inventory * buf_inventory;	/* DOCA Buffer Inventory to hold DOCA buffers */
	struct doca_dev * dev;				/* DOCA Device instance for RegEx */
	struct doca_mmap * mmap;				/* DOCA MMAP to hold DOCA Inventory */
	struct doca_regex * doca_regex;			/* DOCA RegEx instance */
	struct doca_workq * workq;			/* DOCA work queue */
};

/*
 * Structure to hold the various pieces of data pertaining to each job
 */
struct test_dns_job_metadata {
	struct doca_buf *job_data;		/* Pointer to the data to be scanned with this job */
	struct doca_regex_search_result result;	/* Storage for results */
};

#endif  /* _TEST_DOCA_REGEX_H_ */