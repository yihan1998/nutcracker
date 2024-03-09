#ifndef _KERNEL_SCHED_H_
#define _KERNEL_SCHED_H_

#include <stdbool.h>
#include <unistd.h>
#include <sched.h>

#include "init.h"
#include "list.h"
#include "percpu.h"

extern bool kernel_early_boot;
extern bool kernel_shutdown;

extern int __init sched_init(void);

#endif  /* _KERNEL_SCHED_H_ */