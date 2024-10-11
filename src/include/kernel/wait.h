#ifndef _WAIT_H_
#define _WAIT_H_

#include <linux/types.h>

#include "list.h"
#include "kernel/sched.h"

typedef struct wait_queue_entry wait_queue_entry_t;
typedef int (*wait_queue_func_t)(struct wait_queue_entry * wq_entry, unsigned mode, int flags, void * key);

/* wait_queue_entry::flags */
#define WQ_FLAG_EXCLUSIVE	0x01
#define WQ_FLAG_WOKEN		0x02
#define WQ_FLAG_BOOKMARK	0x04

/*
 * A single wait-queue entry structure:
 */
struct wait_queue_entry {
	unsigned int flags;
	void * private;
	wait_queue_func_t func;
	struct list_head entry;
};

typedef struct wait_queue_head {
	// spinlock_t lock;
	struct list_head head;
} wait_queue_head_t;

struct task_struct;

int default_wake_function(struct wait_queue_entry * wq_entry, unsigned mode, int flags, void * key);
int wake_up_task(struct task_struct * task);

/*
 * Macros for declaration and initialisaton of the datatypes
 */
#define __WAITQUEUE_INITIALIZER(name, tsk) {					\
	.private	= tsk,							\
	.func		= default_wake_function,				\
	.entry		= { NULL, NULL } }

#define DECLARE_WAITQUEUE(name, tsk)						\
	struct wait_queue_entry name = __WAITQUEUE_INITIALIZER(name, tsk)

#define __WAIT_QUEUE_HEAD_INITIALIZER(name) {					\
	.lock		= SPIN_LOCK_UNLOCKED(name.lock),			\
	.head		= { &(name).head, &(name).head } }

#define DECLARE_WAIT_QUEUE_HEAD(name) \
	struct wait_queue_head name = __WAIT_QUEUE_HEAD_INITIALIZER(name)

extern void init_waitqueue_head(struct wait_queue_head * wq_head);

static inline void init_waitqueue_func_entry(struct wait_queue_entry * wq_entry, wait_queue_func_t func) {
	wq_entry->flags = 0;
	wq_entry->private = NULL;
	wq_entry->func = func;
}

/**
 * waitqueue_active -- locklessly test for waiters on the queue
 * @wq_head: the waitqueue to test for waiters
 *
 * returns true if the wait list is not empty
 *
 * NOTE: this function is lockless and requires care, incorrect usage _will_
 * lead to sporadic and non-obvious failure.
 *
 * Use either while holding wait_queue_head::lock or when used for wakeups
 * with an extra smp_mb() like::
 *
 *      CPU0 - waker                    CPU1 - waiter
 *
 *                                      for (;;) {
 *      @cond = true;                     prepare_to_wait(&wq_head, &wait, state);
 *      smp_mb();                         // smp_mb() from set_current_state()
 *      if (waitqueue_active(wq_head))         if (@cond)
 *        wake_up(wq_head);                      break;
 *                                        schedule();
 *                                      }
 *                                      finish_wait(&wq_head, &wait);
 *
 * Because without the explicit smp_mb() it's possible for the
 * waitqueue_active() load to get hoisted over the @cond store such that we'll
 * observe an empty wait list while the waiter might not observe @cond.
 *
 * Also note that this 'optimization' trades a spin_lock() for an smp_mb(),
 * which (when the lock is uncontended) are of roughly equal cost.
 */
static inline int waitqueue_active(struct wait_queue_head * wq_head) {
	return !list_empty(&wq_head->head);
}

/**
 * wq_has_sleeper - check if there are any waiting processes
 * @wq_head: wait queue head
 *
 * Returns true if wq_head has waiting processes
 */
static inline bool wq_has_sleeper(struct wait_queue_head * wq_head) {
	return waitqueue_active(wq_head);
}

static inline void __add_wait_queue_entry_tail(struct wait_queue_head * wq_head, struct wait_queue_entry * wq_entry) {
	list_add_tail(&wq_entry->entry, &wq_head->head);
}

void __remove_wait_queue(struct wait_queue_head * wq_head, struct wait_queue_entry * wq_entry);
void remove_wait_queue(struct wait_queue_head * wq_head, struct wait_queue_entry * wq_entry);

void __wake_up_sync_key(struct wait_queue_head * wq_head, unsigned int mode, int nr, void * key);

void __wake_up(struct wait_queue_head * wq_head, unsigned int mode, int nr, void * key);
void __wake_up_locked(struct wait_queue_head * wq_head, unsigned int mode, int nr);
void __wake_up_locked_key(struct wait_queue_head * wq_head, unsigned int mode, void * key);

#define wake_up(x)			__wake_up(x, TASK_NORMAL, 1, NULL)
#define wake_up_nr(x, nr)	__wake_up(x, TASK_NORMAL, nr, NULL)
#define wake_up_all(x)		__wake_up(x, TASK_NORMAL, 0, NULL)
#define wake_up_locked(x)		__wake_up_locked((x), TASK_NORMAL, 1)
#define wake_up_all_locked(x)	__wake_up_locked((x), TASK_NORMAL, 0)

#define wake_up_interruptible_all(x)	__wake_up(x, TASK_INTERRUPTIBLE, 0, NULL)

/*
 * Wakeup macros to be used to report events to the targets.
 */
#define poll_to_key(m) ((void *)(uintptr_t)(__poll_t)(m))
#define key_to_poll(m) ((__poll_t)(uintptr_t)(void *)(m))
#define wake_up_poll(x, m)	\
	__wake_up(x, TASK_NORMAL, 1, poll_to_key(m))
#define wake_up_locked_poll(x, m)						\
	__wake_up_locked_key((x), TASK_NORMAL, poll_to_key(m))
#define wake_up_interruptible_poll(x, m)					\
	__wake_up(x, TASK_INTERRUPTIBLE, 1, poll_to_key(m))
#define wake_up_interruptible_sync_poll(x, m)					\
	__wake_up_sync_key((x), TASK_INTERRUPTIBLE, 1, poll_to_key(m))
#define wake_up_interruptible_sync_poll_all(x, m)					\
	__wake_up_sync_key((x), TASK_INTERRUPTIBLE, 0, poll_to_key(m))

bool prepare_to_wait_exclusive(struct wait_queue_head * wq_head, struct wait_queue_entry * wq_entry, int state);
void finish_wait(struct wait_queue_head * wq_head, struct wait_queue_entry * wq_entry);
int autoremove_wake_function(struct wait_queue_entry * wq_entry, unsigned mode, int sync, void * key);
int woken_wake_function(struct wait_queue_entry * wq_entry, unsigned mode, int sync, void * key);

#define DEFINE_WAIT_FUNC(name, function)					\
	struct wait_queue_entry name = {					\
		.private	= current,					\
		.func		= function,					\
		.entry		= LIST_HEAD_INIT((name).entry),			\
	}

#define DEFINE_WAIT(name) DEFINE_WAIT_FUNC(name, autoremove_wake_function)

#define init_wait(wait)								\
	do {									\
		(wait)->private = current;					\
		(wait)->func = autoremove_wake_function;			\
		init_list_head(&(wait)->entry);					\
		(wait)->flags = 0;						\
	} while (0)

static inline void __add_wait_queue(struct wait_queue_head * wq_head, struct wait_queue_entry * wq_entry) {
	list_add(&wq_entry->entry, &wq_head->head);
}

/*
 * Used for wake-one threads:
 */
static inline void __add_wait_queue_exclusive(struct wait_queue_head * wq_head, struct wait_queue_entry * wq_entry) {
	wq_entry->flags |= WQ_FLAG_EXCLUSIVE;
	__add_wait_queue(wq_head, wq_entry);
}

long wait_woken(struct wait_queue_entry * wq_entry, unsigned mode, long timeout);

void add_wait_queue(struct wait_queue_head * wq_head, struct wait_queue_entry * wq_entry);
void add_wait_queue_exclusive(struct wait_queue_head * wq_head, struct wait_queue_entry * wq_entry);

#define	MAX_SCHEDULE_TIMEOUT		LONG_MAX

#endif  /* _WAIT_H_ */