#ifndef _FILE_H_
#define _FILE_H_

#include "core.h"
#include "cpumask.h"

#define NR_FD_BLOCK 40
#define NR_MAX_FILE (NR_FD_BLOCK * BITS_PER_TYPE(unsigned long))

struct files_struct {
	// spinlock_t file_lock;
    DECLARE_BITMAP(bits, NR_MAX_FILE);
    struct file * fdtab[NR_MAX_FILE];
};

extern struct files_struct * files;

#endif  /* _FILE_H_ */