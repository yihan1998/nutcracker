#define _GNU_SOURCE
#define __USE_GNU
#include <dlfcn.h>

#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#include "init.h"
#include "opt.h"
#include "printk.h"

EVP_MD_CTX *(*libssl_EVP_MD_CTX_new)(void) = NULL;
int (*libssl_EVP_DigestInit_ex)(EVP_MD_CTX *ctx, const EVP_MD *type, ENGINE *impl) = NULL;
int (*libssl_EVP_DigestUpdate)(EVP_MD_CTX *ctx, const void *d, size_t cnt) = NULL;
int (*libssl_EVP_DigestFinal_ex)(EVP_MD_CTX *ctx, unsigned char *md, unsigned int *s) = NULL;
const EVP_MD *(*libssl_EVP_sha256)(void) = NULL;
void (*libssl_ERR_print_errors_fp)(FILE *fp) = NULL;

struct evp_md_info {
    union {
        EVP_MD_CTX * evp;
        void * doca;
    } ctx;
    union {
        const EVP_MD * evp;
        void * doca;
    } type;
};

EVP_MD_CTX *EVP_MD_CTX_new(void) {
    pr_debug(SSL_DEBUG, "%s -> Creating new context...\n", __func__);
    struct evp_md_info * info = (struct evp_md_info *)calloc(1, sizeof(struct evp_md_info));
#ifdef CONFIG_DOCA
#else
    info->ctx.evp = libssl_EVP_MD_CTX_new();
#endif
    return (EVP_MD_CTX *)info;
}

int EVP_DigestInit_ex(EVP_MD_CTX *ctx, const EVP_MD *type, ENGINE *impl) {
    pr_debug(SSL_DEBUG, "%s -> Digest init...\n", __func__);
    struct evp_md_info * info = (struct evp_md_info *)ctx;
#ifdef CONFIG_DOCA
#else
    info->type.evp = type;
    return libssl_EVP_DigestInit_ex(info->ctx.evp, info->type.evp, impl);
#endif
}

int EVP_DigestUpdate(EVP_MD_CTX *ctx, const void *d, size_t cnt) {
    pr_debug(SSL_DEBUG, "%s -> Digest update...\n", __func__);
    struct evp_md_info * info = (struct evp_md_info *)ctx;
#ifdef CONFIG_DOCA
#else
    return libssl_EVP_DigestUpdate(info->ctx.evp, d, cnt);
#endif
}

int EVP_DigestFinal_ex(EVP_MD_CTX *ctx, unsigned char *md, unsigned int *s) {
    pr_debug(SSL_DEBUG, "%s -> Digest final...\n", __func__);
    struct evp_md_info * info = (struct evp_md_info *)ctx;
#ifdef CONFIG_DOCA
#else
    return libssl_EVP_DigestFinal_ex(info->ctx.evp, md, s);
#endif
}

const EVP_MD *EVP_sha256(void) {
    pr_debug(SSL_DEBUG, "%s -> sha256...\n", __func__);
#ifdef CONFIG_DOCA
#else
    return libssl_EVP_sha256();
#endif
}

void ERR_print_errors_fp(FILE *fp) {
    pr_debug(SSL_DEBUG, "%s -> Print errors...\n", __func__);
#ifdef CONFIG_DOCA
#else
    return libssl_ERR_print_errors_fp(fp);
#endif
}

static void * bind_symbol(void * libssl, const char * sym) {
    void * ptr;

    if ((ptr = dlsym(libssl, sym)) == NULL) {
        pr_err("Libssl interpose: dlsym failed (%s)\n", sym);
    }

    return ptr;
}

int __init ssl_init(void) {
    char *error;
    void* libssl = dlopen("libssl.so", RTLD_NOW | RTLD_GLOBAL);
    if (!libssl) {
        fprintf(stderr, "Failed to load preload library: %s\n", dlerror());
        exit(EXIT_FAILURE);
    }

    if ((error = dlerror()) != NULL)  {
        fprintf(stderr, "%s\n", error);
        exit(EXIT_FAILURE);
    }

    libssl_EVP_MD_CTX_new = bind_symbol(libssl, "EVP_MD_CTX_new");
    libssl_EVP_DigestInit_ex = bind_symbol(libssl, "EVP_DigestInit_ex");
    libssl_EVP_DigestUpdate = bind_symbol(libssl, "EVP_DigestUpdate");
    libssl_EVP_DigestFinal_ex = bind_symbol(libssl, "EVP_DigestFinal_ex");
    libssl_EVP_sha256 = bind_symbol(libssl, "EVP_sha256");
    libssl_ERR_print_errors_fp = bind_symbol(libssl, "ERR_print_errors_fp");
    return 0;
}