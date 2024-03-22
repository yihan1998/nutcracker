#ifndef _LIBAIO_H_
#define _LIBAIO_H_

#include <fs/aio.h>

int io_setup(int maxevents, io_context_t * ctxp);
int io_destroy(io_context_t ctx);
int io_submit(io_context_t ctx, long nr, struct iocb * ios[]);

#endif  /* _LIBAIO_H_ */