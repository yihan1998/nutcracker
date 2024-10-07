#ifndef _INTERPOSE_H_
#define _INTERPOSE_H_

#include "init.h"
// #include "lib.h"

extern int __init ipc_init(void);
extern int __init fs_init(void);
extern int __init net_init(void);

#endif  /* _INTERPOSE_H_ */