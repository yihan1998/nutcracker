#define _GNU_SOURCE
#include <dlfcn.h>
#include <stddef.h>

#include "init.h"
#include "printk.h"
#include "libc.h"

bool libc_hooked = false;

int (*libc_regcomp)(regex_t *_Restrict_ __preg,
                const char *_Restrict_ __pattern, int __cflags);
int (*libc_regexec)(const regex_t *_Restrict_ __preg,
                const char *_Restrict_ __String, size_t __nmatch,
                regmatch_t __pmatch[_Restrict_arr_ _REGEX_NELTS (__nmatch)],
                int __eflags);

static void * bind_symbol(const char * sym) {
    void * ptr;
    
    if ((ptr = dlsym(RTLD_NEXT, sym)) == NULL) {
        pr_err("Libc interpose: dlsym failed (%s)\n", sym);
    }

    return ptr;
}

int regex_hook_init(void) {
    libc_regcomp = bind_symbol("regcomp");
    libc_regexec = bind_symbol("regexec");

    return 0;
}

int __init libc_hook_init(void) {
    regex_hook_init();

    libc_hooked = true;

    return 0;
}