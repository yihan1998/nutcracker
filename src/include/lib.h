#ifndef _LIB_H_
#define _LIB_H_

#include "init.h"
#include <zlib.h>

extern int (*libz_deflate)(z_streamp strm, int flush);
extern int (*libz_deflateInit_)(z_streamp strm, int level, const char *version, int stream_size);
extern int (*libz_deflateEnd)(z_streamp strm);
extern int (*libz_inflate)(z_streamp strm, int flush);
extern int (*libz_inflateInit_)(z_streamp strm, const char *version, int stream_size);
extern int (*libz_inflateEnd)(z_streamp strm);

int __init zlib_hook_init(void);

#endif  /* _LIB_H_ */