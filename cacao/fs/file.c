#include <errno.h>
#include <stdlib.h>

#include "opt.h"
#include "printk.h"
#include "fs/fs.h"
#include "fs/file.h"

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

static unsigned int find_next_fd(struct files_struct * files, unsigned int start) {
    unsigned int max_block, start_block;
    
    unsigned long block_bitmap;
    unsigned int next_zero_bit;

    max_block = NR_FD_BLOCK;
    start_block = start / BITS_PER_TYPE(unsigned long);

    if (start >= NR_MAX_FILE) {
        return NR_MAX_FILE;
    }

    next_zero_bit = NR_FD_BLOCK;

    for (int i = start_block; i < max_block; i++) {
        block_bitmap = files->bits[i];

        if (!~block_bitmap) {
            continue;
        }

        next_zero_bit = find_next_zero_bit(&block_bitmap, BITS_PER_TYPE(unsigned long), start % BITS_PER_TYPE(unsigned long));
        next_zero_bit += i * BITS_PER_TYPE(unsigned long);
        pr_debug(FILE_DEBUG, "block %d: %lx, next zero bit: %d\n", i, block_bitmap, next_zero_bit);
        break;
    }

    return next_zero_bit;
}

static int __alloc_fd(struct files_struct * files, unsigned start, unsigned end, unsigned flags) {
    unsigned int fd;

    fd = start;

    if (fd < NR_MAX_FILE) {
        fd = find_next_fd(files, fd);
    }

    if (fd >= end) {
        return -EBADF;
    }
    
    __set_open_fd(files, fd);

    return fd;
}

/**
 * Get one unused file desriptor
 * 
 * @param flags file flags
 * @return file descriptor 
 */
int get_unused_fd_flags(unsigned flags) {
    return __alloc_fd(files, 0, NR_MAX_FILE, flags);
}

/**
 * Put one unused file desriptor back
 * 
 * @param fd file desriptor to be released
 */
void put_unused_fd(unsigned int fd) {
	// struct files_struct * files = current->files;
    // spin_lock(&files->file_lock);
	// __put_unused_fd(files, fd);
    // spin_unlock(&files->file_lock);
}

static void __fd_install(struct files_struct * files, unsigned int fd, struct file * file) {
    struct file ** fdt;
    fdt = files->fdtab;
    fdt[fd] = file;
}

/**
 * fd_install - link a file descriptor with a file structure
 * 
 * @param fd file descriptor
 * @param file file structure
 */
void fd_install(unsigned int fd, struct file * file) {
    __fd_install(files, fd, file);
}

struct file * fget(unsigned int fd) {
	return (files->fdtab[fd]);
}

struct file * alloc_empty_file(int flags) {
    struct file * file = NULL;
	file = (struct file *)calloc(1, sizeof(struct file));
    return file;
}

struct file * alloc_file(int flags, const struct file_operations * fop) {
    struct file * file;

    rte_mempool_get(file_mp, (void *)&file);
    if (!file) {
        return NULL;
    }

    file->f_op = fop;

    return file;
}