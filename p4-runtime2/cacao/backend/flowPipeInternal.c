#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "backend/flowPipeInternal.h"

uint32_t pipe_id = 1;

struct flow_pipe * instantialize_flow_pipe(const char * name) {
    struct flow_pipe * new_pipe = (struct flow_pipe *)calloc(1, sizeof(struct flow_pipe));
    memcpy(new_pipe->name, name, strlen(name));
    new_pipe->id = pipe_id++;
	printf("[%s:%d] New pipe %s created with id: %d: %p\n", __func__, __LINE__, name, new_pipe->id, new_pipe);
    return new_pipe;
}