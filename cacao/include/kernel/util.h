#ifndef _UTIL_H_
#define _UTIL_H_

#ifndef unlikely
#define unlikely(x)    (__builtin_expect(!!(x), 0))
#endif

#ifndef likely
#define likely(x)    (__builtin_expect(!!(x), 1))
#endif

#endif  /* _UTIL_H_ */