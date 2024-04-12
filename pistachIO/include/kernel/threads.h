#ifndef _THREADS_H_
#define _THREADS_H_

#include <pthread.h>

#include "init.h"

#ifndef CONFIG_NR_CPUS
#define CONFIG_NR_CPUS  1
#endif

#define NR_CPUS CONFIG_NR_CPUS

extern pthread_t worker_ids[NR_CPUS];

extern int __init worker_init(void);

#endif  /* _THREADS_H_ */