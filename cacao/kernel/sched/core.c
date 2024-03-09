/*
 * kernel/sched/core.c
 *
 * Core kernel scheduler code and related syscalls
 */
#include <stdlib.h>

#include "percpu.h"
#include "printk.h"
#include "kernel.h"

DEFINE_PER_CPU(unsigned int, cpu_id);