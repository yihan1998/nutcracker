#ifndef _LINUX_REGEX_H_
#define _LINUX_REGEX_H_

#include <regex.h>

extern int load_and_compile_regex_rules(regex_t * rules, int max_rules, const char * file_path);

#endif  /* _LINUX_REGEX_H_ */