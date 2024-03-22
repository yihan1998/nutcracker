#ifndef _POLL_H_
#define _POLL_H_

#include "fs/file.h"
#include "kernel/wait.h"

struct wait_queue_head_t;
struct poll_table_struct;

typedef void (*poll_queue_proc)(struct file *, wait_queue_head_t *, struct poll_table_struct *);

typedef struct poll_table_struct {
    poll_queue_proc qproc;
    __poll_t key;
} poll_table;

static inline bool poll_does_not_wait(const poll_table * p) {
	return p == NULL || p->qproc == NULL;
}

static inline __poll_t poll_requested_events(const poll_table * p) {
    return p ? p->key : ~(__poll_t)0;
}

static inline void init_poll_funcptr(poll_table * pt, poll_queue_proc qproc) {
    pt->qproc = qproc;
    pt->key = ~(__poll_t)0;
}

static inline __poll_t vfs_poll(struct file * file, struct poll_table_struct * pt) {
    return file->f_op->poll(file, pt);
}

static inline void poll_wait(struct file * filp, wait_queue_head_t * wait_address, poll_table * p) {
    if (p && p->qproc && wait_address) {
        p->qproc(filp, wait_address, p);
    }
}

#endif  /* _POLL_H_ */