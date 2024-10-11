#define _GNU_SOURCE
#define __USE_GNU
#include <dlfcn.h>
#include <zlib.h>
#include <stdlib.h>

#include "init.h"
#include "opt.h"
#include "utils/printk.h"
#include "drivers/doca/compress.h"

int (*libz_deflate)(z_streamp strm, int flush) = NULL;
int (*libz_deflateInit_)(z_streamp strm, int level, const char *version, int stream_size)= NULL;
int (*libz_deflateEnd)(z_streamp strm) = NULL;
int (*libz_inflate)(z_streamp strm, int flush) = NULL;
int (*libz_inflateInit_)(z_streamp strm, const char *version, int stream_size)= NULL;
int (*libz_inflateEnd)(z_streamp strm) = NULL;

int deflateInit_(z_streamp strm, int level, const char *version, int stream_size) {
    pr_debug(ZLIB_DEBUG, "%s: strm: %p, level: %d, version: %p, streamSize: %d\n", __func__, strm, level, version, stream_size);
#ifdef CONFIG_DOCA_COMPRESS
    return libz_deflateInit_(strm, level, version, stream_size);
#else
    return libz_deflateInit_(strm, level, version, stream_size);
#endif
}

int deflate(z_streamp strm, int flush) {
    pr_debug(ZLIB_DEBUG, "%s: strm: %p, flush: %d\n", __func__, strm, flush);
#ifdef CONFIG_DOCA_COMPRESS
    if (docadv_deflate(strm, flush) == DOCA_SUCCESS) {
        return Z_STREAM_END;
    }
    return Z_ERRNO;
    
#else
    return libz_deflate(strm, flush);
#endif
}

int deflateEnd(z_streamp strm) {
    pr_debug(ZLIB_DEBUG, "%s: strm: %p\n", __func__, strm);
#ifdef CONFIG_DOCA_COMPRESS
    return libz_deflateEnd(strm);
#else
    return libz_deflateEnd(strm);
#endif
}

int inflateInit_(z_streamp strm, const char *version, int stream_size) {
    pr_debug(ZLIB_DEBUG, "%s: strm: %p, version: %p, streamSize: %d\n", __func__, strm, version, stream_size);
#ifdef CONFIG_DOCA_COMPRESS
    return libz_inflateInit_(strm, version, stream_size);
#else
    return libz_inflateInit_(strm, version, stream_size);
#endif
}

int inflate(z_streamp strm, int flush) {
    // pr_debug(ZLIB_DEBUG, "%s: strm: %p, flush: %d\n", __func__, strm, flush);
#ifdef CONFIG_DOCA_COMPRESS
    if (docadv_inflate(strm, flush) == DOCA_SUCCESS) {
        return Z_STREAM_END;
    }
    return Z_ERRNO;
#else
    return libz_inflate(strm, flush);
#endif
}

int inflateEnd(z_streamp strm) {
    pr_debug(ZLIB_DEBUG, "%s: strm: %p\n", __func__, strm);
#ifdef CONFIG_DOCA_COMPRESS
    return libz_inflateEnd(strm);
#else
    return libz_inflateEnd(strm);
#endif
}

static void * bind_symbol(void * handle, const char * sym) {
    void * ptr;
    char * error;

    if ((ptr = dlsym(handle, sym)) == NULL) {
        error = dlerror();
        pr_err("Libc interpose: dlsym failed (%s): %s\n", sym, error ? error : "Unknown error");
    }

    return ptr;
}

int __init zlib_hook_init(void) {
    void * handle = dlopen("/usr/lib/aarch64-linux-gnu/libz.so", RTLD_LAZY);
    if (!handle) {
        pr_err("Error loading library: %s\n", dlerror());
        return -1;
    }

    libz_deflateInit_ = bind_symbol(handle, "deflateInit_");
    libz_deflate = bind_symbol(handle, "deflate");
    libz_deflateEnd = bind_symbol(handle, "deflateEnd");
    libz_inflateInit_ = bind_symbol(handle, "inflateInit_");
    libz_inflate = bind_symbol(handle, "inflate");
    libz_inflateEnd = bind_symbol(handle, "inflateEnd");

    return 0;
}