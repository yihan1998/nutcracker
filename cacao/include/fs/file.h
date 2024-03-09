#ifndef _FILE_H_
#define _FILE_H_

#include "percpu.h"
#include "kernel/cpumask.h"
#include "fs/fs.h"

#define NR_FD_BLOCK 40
#define NR_MAX_FILE (NR_FD_BLOCK * BITS_PER_TYPE(unsigned long))

struct files_struct {
	// spinlock_t file_lock;
    DECLARE_BITMAP(bits, NR_MAX_FILE);
    struct file * fdtab[NR_MAX_FILE];
};

extern struct files_struct * files;

extern void set_open_fd(unsigned int fd);
extern int get_unused_fd_flags(unsigned flags);
extern void put_unused_fd(unsigned int fd);
extern struct file * alloc_file(int flags, const struct file_operations * fop);
extern void fd_install(unsigned int fd, struct file * file);
extern struct file * fget(unsigned int fd);

#endif  /* _FILE_H_ */