#ifndef _LOADER_H_
#define _LOADER_H_

#include "init.h"

#include <stdbool.h>

extern int loader_fd;

extern int compile_and_run(const char * filepath);
extern int __init loader_init(void);

#endif  /* _LOADER_H_ */