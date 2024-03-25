#include <assert.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "opt.h"
#include "printk.h"

#include "kernel.h"
#include "kernel/poll.h"
#include "fs/aio.h"
#include "fs/fs.h"
#include "net/net.h"

struct rte_mempool * io_ctx_mp;
struct rte_mempool * io_task_mp;

struct rte_ring * io_worker_rq;
struct rte_ring * io_worker_cq;

struct aio_poll_table {
	struct poll_table_struct    pt;
	struct aio_iocb *iocb;
	bool    queued;
	int     error;
};

/* Wrapper struct used by poll queueing */
struct iopoll_pqueue {
	poll_table pt;
	struct iopoll_item * item;
};

static inline struct iopoll_item * iopoll_item_from_wait(wait_queue_entry_t * p) {
	return __container_of__(p, struct iopoll_item, wait);
}

static inline struct iopoll_item * iopoll_item_from_epqueue(poll_table * p) {
	return __container_of__(p, struct iopoll_pqueue, pt)->item;
}

// static struct ioctx * ioctx_alloc(unsigned nr_events) {

// }

int __init io_init(void) {
    io_ctx_mp = rte_mempool_lookup("io_ctx_mp");
    assert(io_ctx_mp != NULL);

    io_task_mp = rte_mempool_lookup("io_task_mp");
    assert(io_task_mp != NULL);

    io_worker_rq = rte_ring_lookup("worker_rq");
    assert(io_worker_rq != NULL);

    io_worker_cq = rte_ring_lookup("worker_cq");
    assert(io_worker_cq != NULL);

    int fd = shm_open("fdtable", O_RDWR, 0666);
    if (fd == -1) {
        perror("Error with shm_open");
        exit(EXIT_FAILURE);
    }

    struct files_struct * fs = mmap(NULL, sizeof(struct files_struct), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fs == MAP_FAILED) {
        perror("Error with mmap");
        exit(EXIT_FAILURE);
    }

    files = fs;

    return 0;
}

int io_setup(int nr_events, io_context_t * ctx_idp) {
    pr_debug(AIO_DEBUG, "%s: nr_event: %d, ctx_idp: %p\n", __func__, nr_events, ctx_idp);
    io_context_t new_ctx = NULL;

	rte_mempool_get(io_ctx_mp, (void *)&new_ctx);
    if (!new_ctx) {
        return -1;
    }

    init_list_head(&new_ctx->cbs);

    *ctx_idp = new_ctx;

    return 0;
}

int io_destroy(io_context_t ctx) {
    pr_debug(AIO_DEBUG, "%s: ctx: %p\n", __func__, ctx);
    return 0;
}

static struct iocb_task_struct * create_io_task(void * argp, void (*func)(void *)) {
    struct iocb_task_struct * new = NULL;

    rte_mempool_get(io_task_mp, (void **)&new);
    if (!new) {
        return NULL;
    }

    new->argp = argp;
    new->func = func;

    rte_ring_enqueue(io_worker_rq, new);

    return new;
}

/**
 * iocb call back funtion - called by wake_up_*() functions
 * * Call with wait queue head lock acquired
*/
static int iocb_poll_callback(struct wait_queue_entry * wait, unsigned mode, int flags, void * key) {
    struct iopoll_item * item = iopoll_item_from_wait(wait);
	__poll_t pollflags = key_to_poll(key);
    struct iocb * iocb = item->iocb;
    struct iocb_task_struct * new = NULL;

    void (*func)(void *) = iocb->data;
    void * argp = iocb->u.c.buf;

    if (pollflags && !(pollflags & item->events)) {
		return 0;
	}

    new = create_io_task(argp, func);
    if (!new) {
        pr_err("Failed to create new task for incoming request!\n");
        return -1;
    }

    return 0;
}

static __poll_t iocb_poll(struct iopoll_item * entry, poll_table * pt) {
    pt->key = entry->events;
	return vfs_poll(entry->ffd.file, pt) & entry->events;
}

static void iocb_ptable_queue_proc(struct file * file, wait_queue_head_t * whead, poll_table * pt) {
	struct iopoll_item * item = iopoll_item_from_epqueue(pt);

	init_waitqueue_func_entry(&item->wait, iocb_poll_callback);
	item->whead = whead;
	if (item->events & EPOLLEXCLUSIVE) {
		add_wait_queue_exclusive(whead, &item->wait);
	} else {
        pr_debug(AIO_DEBUG, "%s: add iopoll item %p(wq entry: %p) to whead %p\n", __func__, item, &item->wait, whead);
		add_wait_queue(whead, &item->wait);
	}
}

static int io_submit_one(struct io_context * ctx, struct iocb * iocb) {
    int fd;
    struct file * tfile;
    struct iopoll_pqueue pqueue;
    struct iopoll_item * item;
	__poll_t revents;

    fd = iocb->aio_fildes;

    item = (struct iopoll_item *)calloc(1, sizeof(struct iopoll_item));

    /* Get the "struct file *" for the target file */
	tfile = fget(fd);
	if (!tfile) {
		pr_err("Failed to find the target file!\n");
        return 0;
    }

    item->ctx = ctx;
    item->iocb = iocb;
	item->ffd.file = tfile;
	item->ffd.fd = fd;
	item->events = (iocb->aio_lio_opcode == IO_CMD_PREAD)? EPOLLIN : 0;

	init_poll_funcptr(&pqueue.pt, iocb_ptable_queue_proc);
    pqueue.item = item;

	revents = iocb_poll(item, &pqueue.pt);
	if (revents) {
		pr_err("Failed to find the target file!\n");
    }

    return 0;
}

int io_submit(io_context_t ctx_id, long nr, struct iocb ** iocbpp) {
    int ret, total = 0;
    struct io_context * ctx = ctx_id;

    pr_debug(AIO_DEBUG, "%s: ctx_id: %p, nr: %d, iocbpp: %p\n", __func__, ctx_id, nr, iocbpp);
    for (int i = 0; i < nr; i++) {
        pr_debug(AIO_DEBUG, "sockfd: %d, OP: %d, data: %p, buf: %p, cbx: %p\n", 
                iocbpp[i]->aio_fildes, iocbpp[i]->aio_lio_opcode, iocbpp[i]->data, 
                iocbpp[i]->u.c.buf, iocbpp);

		ret = io_submit_one(ctx, iocbpp[i]);
		if (ret) {
			break;
        } else {
            total++;
        }
    }

	return total;
}
