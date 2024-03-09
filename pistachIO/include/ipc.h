#ifndef _IPC_H_
#define _IPC_H_

#include "init.h"

extern int __init ipc_init(void);

extern int control_fd;

typedef enum {
    REQUEST_CORE = 0x1, 
    RELEASE_CORE, 
    REQUEST_COREMASK,
    RELEASE_COREMASK,
} msg_type_t;

struct ipc_msghdr {
    msg_type_t      type;
    unsigned int    src_core;
    unsigned int    dst_core;
    int             len;
};

#endif  /* _IPC_H_ */