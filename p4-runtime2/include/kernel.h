#ifndef _KERNEL_H_
#define _KERNEL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef max
#define max(x, y) ({				\
	typeof(x) _x = (x);			\
	typeof(y) _y = (y);			\
	_x > _y ? _x : _y; })
#endif

#ifndef min
#define min(x, y) ({				\
	typeof(x) _x = (x);			\
	typeof(y) _y = (y);			\
	_x < _y ? _x : _y; })
#endif

/**
 * __container_of__ - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define __container_of__(ptr, type, member) ({				\
	void *__mptr = (void *)(ptr);					\
	((type *)(__mptr - offsetof(type, member))); })

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/**
 * roundup - round up to the next specified multiple
 * @x: the value to up
 * @y: multiple to round up to
 *
 * Rounds @x up to next multiple of @y. If @y will always be a power
 * of 2, consider using the faster round_up().
 */
#define roundup(x, y) (					\
{							\
	typeof(y) __y = y;				\
	(((x) + (__y - 1)) / __y) * __y;		\
}							\
)
/**
 * rounddown - round down to next specified multiple
 * @x: the value to round
 * @y: multiple to round down to
 *
 * Rounds @x down to next multiple of @y. If @y will always be a power
 * of 2, consider using the faster round_down().
 */
#define rounddown(x, y) (				\
{							\
	typeof(x) __x = (x);				\
	__x - (__x % (y));				\
}							\
)

/**
 * Align a value to a given power of two (get round)
 * @result
 *      Aligned value that is no bigger than value
 */
#define ALIGN_ROUND(val, align) \
    (typeof(val))((val) & (~((typeof(val))((align) - 1))))

/**
 * Align a value to a given power of two (get ceiling)
 * @result
 *      Aligned value that is no smaller than value
 */
#define ALIGN_CEIL(val, align) \
    ALIGN_ROUND(val + (typeof(val))((align) - 1), align)

#define NSEC_PER_SEC        1000000000ULL
#define TIMESPEC_TO_NSEC(t) ((t.tv_sec * NSEC_PER_SEC) + (t.tv_nsec))

#define READ_ONCE(x)	atomic_load_explicit(&(x), memory_order_relaxed)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* _KERNEL_H_ */