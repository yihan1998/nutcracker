#ifndef _DOCA_COMPRESS_H_
#define _DOCA_COMPRESS_H_

#include <zlib.h>

#ifdef CONFIG_DOCA_COMPRESS
#include "doca/common.h"
#include <doca_compress.h>

struct docadv_compress_config {
	char pci_address[DOCA_DEVINFO_PCI_ADDR_SIZE];   /* RegEx PCI address to use */
	struct doca_dev * dev;  /* DOCA device */
	struct doca_compress * compress_engine;   /* DOCA RegEx interface */
};

extern struct docadv_compress_config compress_config;

struct docadv_compress_ctx {
	struct buf_info src_buf;
	struct buf_info dst_buf;

	struct doca_buf_inventory * buf_inv;    /* DOCA buffer inventory */
	struct doca_dev * dev;		/* DOCA device */
	struct doca_compress * compress_engine;	/* DOCA RegEx interface */
};

extern int docadv_deflate(z_streamp strm, int flush);
extern int docadv_deflateInit_(z_streamp strm, int level, const char *version, int stream_size);
extern int docadv_deflateEnd(z_streamp strm);
extern int docadv_inflate(z_streamp strm, int flush);
extern int docadv_inflateInit_(z_streamp strm, const char *version, int stream_size);
extern int docadv_inflateEnd(z_streamp strm);

doca_error_t docadv_compress_init(void);

#endif  /* CONFIG_DOCA_COMPRESS */

#endif  /* _DOCA_COMPRESS_H_ */