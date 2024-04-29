#include <regex.h>
#include "libc.h"

#ifdef CONFIG_DOCA_REGEX
#include "doca/regex.h"
#endif

int regcomp(regex_t *_Restrict_ __preg, const char *_Restrict_ __pattern, int __cflags) {
#ifndef CONFIG_DOCA_REGEX
    return libc_regcomp(__preg, __pattern, __cflags);
#else
    return doca_regcomp(__preg, __pattern, __cflags);
#endif
}

int regexec(const regex_t *_Restrict_ __preg, const char *_Restrict_ __String, size_t __nmatch,
			regmatch_t __pmatch[_Restrict_arr_ _REGEX_NELTS (__nmatch)], int __eflags) {
#ifndef CONFIG_DOCA_REGEX
    return libc_regexec(__preg, __String, __nmatch, __pmatch, __eflags);
#else
    return doca_regexec(__preg, __String, __nmatch, __pmatch, __eflags);
#endif
}