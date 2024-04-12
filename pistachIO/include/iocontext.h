#ifndef _IOCONTEXT_H_
#define _IOCONTEXT_H_

#include "kernel/wait.h"
#include "fs/aio.h"

#define NR_MAX_IO_CTX   1024
#define NR_MAX_IO_TASK  2048

struct io_context {
    struct list_head    cbs;
};

struct iocb_filefd {
    struct file * file;
    int         fd;
} __attribute__ ((__packed__));

struct iopoll_item {
    struct list_head    list;

	__poll_t            events;

	struct io_context   * ctx;

    struct iocb         * iocb;

    struct iocb_filefd  ffd;

	wait_queue_entry_t  wait;

	wait_queue_head_t   * whead;
};

struct iocb_task_struct {
    void    * argp;
    void    (*func)(void *);
};

#endif  /* _IOCONTEXT_H_ */