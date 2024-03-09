#include <errno.h>
#include <stdlib.h>

#include "opt.h"
#include "printk.h"
#include "fs.h"
#include "file.h"

/* File table (locate in shared memory) */
struct files_struct * files;

static inline void __set_open_fd(struct files_struct * files, unsigned int fd) {
    set_bit(fd % BITS_PER_TYPE(unsigned long), &files->bits[fd / BITS_PER_TYPE(unsigned long)]);
}

static inline void __clear_open_fd(struct files_struct * files, unsigned int fd) {
	clear_bit(fd % BITS_PER_TYPE(unsigned long), &files->bits[fd / BITS_PER_TYPE(unsigned long)]);
}

void set_open_fd(unsigned int fd) {
    __set_open_fd(files, fd);
}