#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <linux/if_ether.h>

#include "utils/printk.h"
#include "utils/args.h"
#include "net/net.h"
#include "backend/flowPipeInternal.h"
#include "backend/stateMachine.h"
#include "backend/stateMachineInternal.h"

extern "C" int fsm_create_table_pipe(struct flow_pipe_cfg* pipe_cfg, struct flow_fwd* fwd, struct flow_pipe* pipe) {
    struct flow_pipe *defaultNext = nullptr;
    if (fwd) {
        if (fwd->type == FLOW_FWD_PIPE) {
            defaultNext = fwd->next_pipe;
        }
    }
    auto table = new TablePipe(pipe_cfg->attr.name,defaultNext);

    std::cout << "Number of current entries: " << table->entries.size() << std::endl;

    pipe->swPipe.pipe = table;
    if (pipe_cfg->attr.domain == FLOW_PIPE_DOMAIN_INGRESS) {
        list_add_tail(&pipe->swPipe.list, &fsm.ingress_pipes);
        if (pipe_cfg->attr.is_root && !fsm.ingress_root) {
            printf("Set ingress root pipe: %s\n", pipe->name);
            fsm.ingress_root = pipe;
        }
    } else if (pipe_cfg->attr.domain == FLOW_PIPE_DOMAIN_EGRESS) {
        list_add_tail(&pipe->swPipe.list, &fsm.egress_pipes);
        if (pipe_cfg->attr.is_root && !fsm.egress_root) {
            printf("Set egress root pipe: %s\n", pipe->name);
            fsm.egress_root = pipe;
        }
    }

    if (pipe_cfg->match) {
        if (pipe_cfg->match->meta.pkt_meta) {
            table->match.meta.pkt_meta = pipe_cfg->match->meta.pkt_meta;
        }
        if (pipe_cfg->match->outer.udp.dest) {
            table->match.outer.udp.dest = pipe_cfg->match->outer.udp.dest;
        }
    }

    return 0;
}

extern "C" int fsm_create_conditional_pipe(struct flow_pipe_cfg* pipe_cfg, struct flow_fwd* fwd, struct flow_pipe* pipe) {
    auto conditional = new ConditionalPipe(pipe_cfg->attr.name);
    pipe->swPipe.pipe = conditional;
    if (pipe_cfg->attr.domain == FLOW_PIPE_DOMAIN_INGRESS) {
        list_add_tail(&pipe->swPipe.list, &fsm.ingress_pipes);
        if (pipe_cfg->attr.is_root && !fsm.ingress_root) {
            printf("Set ingress root pipe: %s\n", pipe->name);
            fsm.ingress_root = pipe;
        }
    } else if (pipe_cfg->attr.domain == FLOW_PIPE_DOMAIN_EGRESS) {
        list_add_tail(&pipe->swPipe.list, &fsm.egress_pipes);
        if (pipe_cfg->attr.is_root && !fsm.egress_root) {
            printf("Set egress root pipe: %s\n", pipe->name);
            fsm.egress_root = pipe;
        }
    }
    return 0;
}
