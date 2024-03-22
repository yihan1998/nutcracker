#include "libaio.h"

#include <fs/aio.h>

// int io_setup(int maxevents, io_context_t * ctxp) {
//     pr_debug(AIO_DEBUG, "%s> nr_event: %d, ctx_idp: %p\n", __func__, nr_events, ctx_idp);
//     return aio_setup(maxevents, ctxp);
// }

// int io_destroy(io_context_t ctx) {
//     pr_debug(AIO_DEBUG, "%s> ctx: %p\n", __func__, ctx);
//     return aio_destroy(ctx);
// }

// int io_submit(io_context_t ctx, long nr, struct iocb * ios[]) {
//     pr_debug(AIO_DEBUG, "%s> ctx_id: %d, nr: %d, iocbpp: %p\n", __func__, ctx_id, nr, iocbpp);
//     return aio_submit(ctx, nr, ios);
// }