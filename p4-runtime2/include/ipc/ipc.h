#ifndef _IPC_H_
#define _IPC_H_

#include "init.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int epfd;
extern int control_fd;

extern int ipc_poll(void);
extern int __init ipc_init(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* _IPC_H_ */