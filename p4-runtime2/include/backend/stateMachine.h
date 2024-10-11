#ifndef _STATEMACHINE_H_
#define _STATEMACHINE_H_

#include "init.h"
#include "list.h"
#include "flowPipeInternal.h"

#ifdef __cplusplus
extern "C" {
#endif

struct state_machine {
    struct flow_pipe * ingress_root;
    struct list_head ingress_pipes;
    struct flow_pipe * egress_root;
    struct list_head egress_pipes;
} __attribute__ ((aligned(64)));

extern struct state_machine fsm;

int __init state_machine_init(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* _STATEMACHINE_H_ */