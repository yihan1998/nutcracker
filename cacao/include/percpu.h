#ifndef _PERCPU_H_
#define _PERCPU_H_

#include "percpu-defs.h"

DECLARE_PER_CPU(unsigned int, cpu_id);

#define get_cpu_var(var, cpu)   (var[cpu])
#define get_cpu_ptr(var, cpu)   (&(var[cpu]))

#define per_cpu_var(var)  (var[cpu_id])
#define per_cpu_ptr(var)  (&(var[cpu_id]))

#endif  /* _PERCPU_H_ */