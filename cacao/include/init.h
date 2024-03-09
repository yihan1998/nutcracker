#ifndef _INIT_H_
#define _INIT_H_

#define __init
#define __exit
#define __sched __attribute__ ((__target__ ("no-sse")))

extern int (*main_orig)(int, char **, char **);
extern int main_argc;
extern char ** main_argv;
extern char ** main_envp;

extern int main_entry(void);

#endif  /* _INIT_H_ */