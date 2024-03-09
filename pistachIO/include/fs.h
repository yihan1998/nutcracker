#ifndef _FS_H_
#define _FS_H_

#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>

#include "init.h"
#include "bitops.h"

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

extern void set_open_fd(unsigned int fd);

extern int __init fs_init(void);

#endif  /* _FS_H_ */