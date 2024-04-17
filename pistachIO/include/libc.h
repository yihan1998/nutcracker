#ifndef _LIBC_H_
#define _LIBC_H_

#include <stdbool.h>
#include <regex.h>

extern bool libc_hooked;

extern int libc_hook_init(void);

extern int (*libc_regcomp)(regex_t *_Restrict_ __preg,
                    const char *_Restrict_ __pattern, int __cflags);
extern int (*libc_regexec)(const regex_t *_Restrict_ __preg,
                    const char *_Restrict_ __String, size_t __nmatch,
                    regmatch_t __pmatch[_Restrict_arr_ _REGEX_NELTS (__nmatch)],
                    int __eflags);

#endif  /* _LIBC_H_ */