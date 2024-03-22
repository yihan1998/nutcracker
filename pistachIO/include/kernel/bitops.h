#ifndef _BITSOPS_H_
#define _BITSOPS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

#include "hweight.h"

#define BITS_PER_BYTE   8

#define BITS_PER_LONG __WORDSIZE

#define DIV_ROUND_UP(n, d)  (((n) + (d) - 1) / (d))

#define BIT_MASK(nr)		(1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)		((nr) / BITS_PER_LONG)
#define BITS_PER_TYPE(type) (sizeof(type) * BITS_PER_BYTE)
#define BITS_TO_LONGS(nr)   DIV_ROUND_UP(nr, BITS_PER_TYPE(long))

extern unsigned long find_first_bit(const unsigned long *addr, unsigned long size);

static inline bool test_bit(int nr, void * addr) {
	int ret = 0;
    asm volatile("bt %2, %1\n\t"
				"sbb %%eax,%%eax\n\t"
				"mov %%eax, %0"
				: "=m" (ret), "+m" (*(uint32_t *)addr)
				: "Ir" (nr));
	return (ret == -1);
}

static inline void set_bit(int nr, volatile void * addr) {
#if defined(__x86_64__) || defined(__i386__)
    asm volatile("btsl %1, %0" : "+m" (*(uint32_t *)addr) : "Ir" (nr));
#elif defined(__arm__) || defined(__aarch64__)
	// Cast the address to an atomic type pointer for atomic operations
    atomic_uint *atomic_addr = (atomic_uint *)addr;
    // Calculate the bit mask for the bit to set
    unsigned int mask = 1U << nr;
    // Atomically OR the mask with the current value to set the bit
    // atomic_fetch_or_explicit performs an atomic bitwise OR and returns the original value
    // The memory_order_relaxed argument specifies that we don't need any specific memory ordering guarantees
    atomic_fetch_or_explicit(atomic_addr, mask, memory_order_relaxed);
#else
	pr_err("Unknown architecture!\n");
#endif
}

static inline void clear_bit(int nr, volatile void * addr) {
#if defined(__x86_64__) || defined(__i386__)
    asm volatile("btrl %1, %0" : "+m" (*(uint32_t *)addr) : "Ir" (nr));
#elif defined(__arm__) || defined(__aarch64__)
	// Cast the address to an atomic type pointer for atomic operations
    atomic_uint *atomic_addr = (atomic_uint *)addr;
    
    // Calculate the bit mask for the bit to clear
    unsigned int mask = ~(1U << nr);
    
    // Atomically AND the mask with the current value to clear the bit
    // atomic_fetch_and_explicit performs an atomic bitwise AND and returns the original value
    // The memory_order_relaxed argument specifies that we don't need any specific memory ordering guarantees
    atomic_fetch_and_explicit(atomic_addr, mask, memory_order_relaxed);
#else
	pr_err("Unknown architecture!\n");
#endif
}

/**
 * __test_and_clear_bit - Clear a bit and return its old value
 * @nr: Bit to clear
 * @addr: Address to count from
 *
 * This operation is non-atomic and can be reordered.
 * If two examples of this operation race, one can appear to succeed
 * but actually fail.  You must protect multiple accesses with a lock.
 */
static inline int test_and_clear_bit(int nr, volatile unsigned long * addr) {
	unsigned long mask = BIT_MASK(nr);
	unsigned long * p = ((unsigned long *)addr) + BIT_WORD(nr);
	unsigned long old = * p;

	*p = old & ~mask;
	return (old & mask) != 0;
}

#define for_each_set_bit(bit, addr, size) \
	for ((bit) = find_first_bit((addr), (size));		\
	     (bit) < (size);					\
	     (bit) = find_next_bit((addr), (size), (bit) + 1))

static inline unsigned long __ffs(unsigned long word) {
	return __builtin_ctzl(word);
}

extern unsigned long find_next_bit(const unsigned long * addr, unsigned long size, unsigned long offset);
extern unsigned long find_next_zero_bit(const unsigned long * addr, unsigned long size, unsigned long offset);

static __always_inline unsigned long hweight_long(unsigned long w) {
	return sizeof(w) == 4 ? hweight32(w) : hweight64((uint64_t)w);
}

#endif /* _BITSOPS_H_ */