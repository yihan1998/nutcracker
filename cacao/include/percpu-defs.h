#ifndef _PERCPU_DEFS_H_
#define _PERCPU_DEFS_H_

/**
 * percpu-defs.h - basic definitions for percpu areas
 *
 * DO NOT INCLUDE DIRECTLY OUTSIDE PERCPU IMPLEMENTATION PROPER.
 *
 * This file is separate from percpu.h to avoid cyclic inclusion
 * dependency from arch header files.
 *
 * This file includes macros necessary to declare percpu sections and
 * variables, and definitions of percpu accessors and operations.  It
 * should provide enough percpu features to arch header files even when
 * they can only include percpu.h to avoid cyclic inclusion dependency.
 */

#define DEFINE_PER_CPU(type, name)  \
    __thread __typeof__(type) name

#define DECLARE_PER_CPU(type, name)  \
    extern __thread __typeof__(type) name

#define DEFINE_PER_CPU_SHARED(type, name)  \
    __typeof__(type) name

#define DECLARE_PER_CPU_SHARED(type, name)  \
    extern __typeof__(type) name

#define DEFINE_PER_CPU_SHARED_ALIGNED(type, name)  \
    __typeof__(type) name __attribute__ ((aligned(64)))

#define DECLARE_PER_CPU_SHARED_ALIGNED(type, name)  \
    extern __typeof__(type) name __attribute__ ((aligned(64)))

#endif  /* _PERCPU_DEFS_H_ */