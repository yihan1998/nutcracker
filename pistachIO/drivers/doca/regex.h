#ifndef _DOCA_REGEX_H_
#define _DOCA_REGEX_H_

#include <doca_buf.h>
#include <doca_buf_inventory.h>
#include <doca_ctx.h>
#include <doca_dev.h>
#include <doca_flow.h>
#include <doca_regex.h>
#include <doca_mmap.h>

#include "doca/common.h"

#define MAX_FILE_NAME	255	/* Maximal length of file path */

/* DNS configuration structure */
struct doca_regex_config {
	char pci_address[DOCA_DEVINFO_PCI_ADDR_SIZE];   /* RegEx PCI address to use */
	struct doca_dev * dev;  /* DOCA device */
	struct doca_regex * doca_reg;   /* DOCA RegEx interface */
};

struct doca_regex_ctx {
	struct doca_buf_inventory * buf_inv;    /* DOCA buffer inventory */
	char * data_buffer;		/* Data buffer */
	size_t data_buffer_len;	/* Data buffer length */
	struct doca_buf * buf;		/* DOCA buf */
	struct doca_mmap * mmap;	/* DOCA mmap */
	struct doca_dev * dev;		/* DOCA device */
	struct doca_regex * doca_reg;	/* DOCA RegEx interface */
};

extern struct doca_regex_config doca_regex_cfg;

#ifndef _REGEX_NELTS
#define _REGEX_NELTS(x)
#endif

extern int doca_regcomp(regex_t *_Restrict_ __preg, const char *_Restrict_ __pattern, int __cflags);
extern int doca_regexec(const regex_t *_Restrict_ __preg, const char *_Restrict_ __String, size_t __nmatch,
			regmatch_t __pmatch[_Restrict_arr_ _REGEX_NELTS (__nmatch)], int __eflags);

extern doca_error_t doca_regex_percore_init(struct doca_regex_ctx * regex_ctx);
extern doca_error_t doca_regex_init(void);

#endif  /* _DOCA_REGEX_H_ */