#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sched.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <link.h>

#include "init.h"
#include "percpu.h"
#include "printk.h"
#include "fs/fs.h"
#include "kernel/util.h"
#include "net/netif.h"
#include "net/net.h"
#include "syscall.h"

bool kernel_early_boot = true;
bool kernel_shutdown = false;

/* Original main() entry and parameters */
int (*main_orig)(int, char **, char **);
int main_argc;
char ** main_argv;
char ** main_envp;

void __exit kernel_power_off(void) {
	/* Close IPC connection */
    pr_info("shutdown: shutting IPC subsystem...\n");

	kernel_shutdown = true;

	libc_exit(EXIT_SUCCESS);
}

/* Actual entry point to invole main() */
int main_entry(void) {
	main_orig(main_argc, main_argv, main_envp);

	pr_info("shutdown: kernel shutdown phase\n");

	kernel_power_off();
	
	return 0;
}

#if 0
static int iterator(struct dl_phdr_info *info, size_t size, void *data) {
    const char  *name;
    size_t       headers, h;

    /* Empty name refers to the binary itself. */
    if (!info->dlpi_name || !info->dlpi_name[0])
        name = (const char *)data;
    else
        name = info->dlpi_name;

    headers = info->dlpi_phnum;
    for (h = 0; h < headers; h++)
        if ((info->dlpi_phdr[h].p_type == PT_LOAD) &&
            (info->dlpi_phdr[h].p_memsz > 0) &&
            (info->dlpi_phdr[h].p_flags)) {
            const uintptr_t  first = info->dlpi_addr + info->dlpi_phdr[h].p_vaddr;
            const uintptr_t  last  = first + info->dlpi_phdr[h].p_memsz - 1;

            /* Addresses first .. last, inclusive, belong to binary 'name'. */
            printf("%s: %lx .. %lx\n", name, (unsigned long)first, (unsigned long)last);
        }

    return 0;
}
#endif

/**
 * start_kernel - init all relative modules in kernel
 * @param argv
 */
void start_kernel(void * arg) {
	if (unlikely(!kernel_early_boot)) {
		return;
	}

    pr_info("init: starting DPDK...\n");
	dpdk_init();

	kernel_early_boot = false;

	pr_info("init: initializing fs...\n");
    fs_init();

    pr_info("init: initializing net...\n");
    net_init();

	/* Start the main thread */
    pr_info("init: starting main thread...\n");
    main_entry();
}

/* Our fake main() that gets called by __libc_start_main() */
int main_hook(int argc, char ** argv, char ** envp) {
	/* Get which CPU we are spawned on */
	cpu_id = sched_getcpu();

	// if (dl_iterate_phdr(iterator, argv[0])) {
    //     fprintf(stderr, "dl_iterate_phdr() failed.\n");
    //     exit(EXIT_FAILURE);
    // }

	pr_info("init: starting Cacao...\n");

	pr_info("init: kernel initialization phase\n");

	main_argc = argc;
	main_argv = argv;
	main_envp = envp;

	start_kernel(NULL);

    return 0;
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