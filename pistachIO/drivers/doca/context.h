#ifndef _DOCA_CONTEXT_H_
#define _DOCA_CONTEXT_H_

#include "init.h"
#include "percpu.h"

struct worker_context;

extern int __init doca_percore_init(struct worker_context * ctx);
extern int __init doca_worker_init(struct worker_context * ctx);
extern int __init doca_init(void);

#endif  /* _DOCA_CONTEXT_H_ */