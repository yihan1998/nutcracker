#ifndef _FS_H_
#define _FS_H_

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>

#include <rte_mempool.h>

#include "init.h"

extern struct rte_mempool * socket_mp;
extern struct rte_mempool * file_mp;

struct file {
    const struct file_operations * f_op;
    union {
        int fd;
        void * ptr;
    } f_priv_data;
    char            f_path[64];
	unsigned int    f_flags;
    unsigned int    f_count;
};

struct file_operations {
    ssize_t (*read)(struct file *, char *, size_t, loff_t *);
    ssize_t (*write)(struct file *, const char *, size_t, loff_t *);
    int (*cntl)(struct file *, int, unsigned long);
    FILE * (*fdopen)(struct file *, const char *);
	int (*open)(struct file *, mode_t);
	int (*openat)(struct file *, mode_t);
	int (*stat)(struct file *, struct stat *);
	int (*release)(struct file *);
};

extern int __init fs_init(void);

#endif  /* _FS_H_ */