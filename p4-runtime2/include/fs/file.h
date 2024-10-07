#ifndef _FILE_H_
#define _FILE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#include "kernel/types.h"

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

struct poll_table_struct;

struct file_operations {
    ssize_t (*read)(struct file *, char *, size_t, loff_t *);
    ssize_t (*write)(struct file *, const char *, size_t, loff_t *);
	__poll_t (*poll)(struct file *, struct poll_table_struct *);
    int (*cntl)(struct file *, int, unsigned long);
    FILE * (*fdopen)(struct file *, const char *);
	int (*open)(struct file *, mode_t);
	int (*openat)(struct file *, mode_t);
	// int (*stat)(struct file *, struct stat *);
	int (*release)(struct file *);
};

extern void set_open_fd(unsigned int fd);
extern int get_unused_fd_flags(unsigned flags);
extern struct file * alloc_file(int flags, const struct file_operations * fop);
extern void fd_install(unsigned int fd, struct file * file);
extern struct file * fget(unsigned int fd);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* _FILE_H_ */