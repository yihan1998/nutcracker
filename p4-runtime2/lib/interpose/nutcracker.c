#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <dlfcn.h>
#include <link.h>

#include "opt.h"
#include "percpu.h"
#include "interpose/interpose.h"
#include "net/dpdk.h"
#include "kernel/sched.h"
#ifdef CONFIG_DOCA
#include "drivers/doca/context.h"
#endif  /* CONFIG_DOCA */
#include "utils/printk.h"

bool kernel_early_boot = true;

struct lcore_context ctx;

/* Original main() entry and parameters */
int (*main_orig)(int, char **, char **);
int main_argc;
char ** main_argv;
char ** main_envp;

/* Our fake main() that gets called by __libc_start_main() */
int main_hook(int argc, char ** argv, char ** envp) {
	/* Get which CPU we are spawned on */
	cpu_id = sched_getcpu();

	pr_info("INIT: starting Cygnus...\n");

	pr_info("INIT: kernel initialization phase\n");

	main_argc = argc;
	main_argv = argv;
	main_envp = envp;

	char * args[] = {
		"", "-c1", "-n4", "-a", "auxiliary:mlx5_core.sf.3,dv_flow_en=2", "-a", "auxiliary:mlx5_core.sf.5,dv_flow_en=2", "--proc-type=secondary"
	};

    pr_info("INIT: initialize DPDK...\n");
    dpdk_init(8, args);

    // pr_info("INIT: connect to control plane...\n");
    // ipc_init();

#ifdef CONFIG_DOCA
    pr_info("INIT: initialize DOCA...\n");
    docadv_init();
#endif  /* CONFIG_DOCA */

#ifdef CONFIG_DOCA
    pr_info("INIT: initialize DOCA driver...\n");
    docadv_ctx_init(&ctx.docadv_ctx);
    docadv_ctx_fetch(&ctx.docadv_ctx);
#endif  /* CONFIG_DOCA */

    // pr_info("init: initializing zlib...\n");
    // zlib_hook_init();

    kernel_early_boot = false;

    cpu_set_t set;
    CPU_ZERO(&set);
    // Set CPU 0 and CPU 1
    CPU_SET(1, &set); // Allow execution on CPU 1

    // Set the CPU affinity for the current process
    if (sched_setaffinity(getpid(), sizeof(cpu_set_t), &set) == -1) {
        perror("sched_setaffinity");
        exit(EXIT_FAILURE);
    }

    pr_info("INIT: initialize fs...\n");
    fs_init();

    pr_info("INIT: initialize net...\n");
    net_init();

    // int len = compress_deflate2(msg, strlen(msg), out);

    // compress_inflate2(out, len);

    return main_orig(main_argc, main_argv, main_envp);
}

/**
 * 	Wrapper for __libc_start_main() that replaces the real main function 
 * 	@param main - original main function
 * 	@param init	- initialization for main
 */
int __libc_start_main(	int (*main)(int, char **, char **), int argc, char **argv, 
						int (*init)(int, char **, char **), void (*fini)(void), void (*rtld_fini)(void), void * stack_end) {
    /* Save the real main function address */
    main_orig = main;

    /* Find the real __libc_start_main()... */
    typeof(&__libc_start_main) orig = dlsym(RTLD_NEXT, "__libc_start_main");

    /* ... and call it with custom main function */
    return orig(main_hook, argc, argv, init, fini, rtld_fini, stack_end);
}