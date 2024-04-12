#include "opt.h"
#include "printk.h"
#include "kernel/wait.h"

void init_waitqueue_head(struct wait_queue_head * wq_head) {
    init_list_head(&wq_head->head);
}

static void __wake_up_common(struct wait_queue_head * wq_head, unsigned int mode, int nr_exclusive, int wake_flags, void * key) {
    wait_queue_entry_t * curr, * next;
    list_for_each_entry_safe(curr, next, &wq_head->head, entry) {
        unsigned flags = curr->flags;
        int ret;
        pr_debug(WAIT_DEBUG, "%s: waking up wq entry %p\n", __func__, curr);
		ret = curr->func(curr, mode, wake_flags, key);
        if (ret < 0) {
            break;
        }

		if (ret && (flags & WQ_FLAG_EXCLUSIVE) && !--nr_exclusive) {
            break;
        }
    }
}

static void __wake_up_common_lock(struct wait_queue_head * wq_head, unsigned int mode, int nr_exclusive, int wake_flags, void * key) {
    __wake_up_common(wq_head, mode, nr_exclusive, 0, key);
}

/**
 * __wake_up_sync_key - wake up threads blocked on a waitqueue.
 */
void __wake_up_sync_key(struct wait_queue_head * wq_head, unsigned int mode, int nr_exclusive, void * key) {
	int wake_flags = 1;

    if (!wq_head) {
        return;
    }
    
    if (nr_exclusive != 1) {
        wake_flags = 0;
    }

    // preempt_disable();
	__wake_up_common_lock(wq_head, mode, nr_exclusive, wake_flags, key);
    // preempt_enable();
}

void __remove_wait_queue(struct wait_queue_head * wq_head, struct wait_queue_entry * wq_entry) {
	list_del(&wq_entry->entry);
}

void remove_wait_queue(struct wait_queue_head * wq_head, struct wait_queue_entry * wq_entry) {
	__remove_wait_queue(wq_head, wq_entry);
}

void add_wait_queue(struct wait_queue_head * wq_head, struct wait_queue_entry * wq_entry) {
    wq_entry->flags &= ~WQ_FLAG_EXCLUSIVE;
    __add_wait_queue_exclusive(wq_head, wq_entry);
}

void add_wait_queue_exclusive(struct wait_queue_head * wq_head, struct wait_queue_entry * wq_entry) {
    wq_entry->flags |= WQ_FLAG_EXCLUSIVE;
    __add_wait_queue_exclusive(wq_head, wq_entry);
}