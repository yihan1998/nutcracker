/*
 * kernel/printk.c
 *
 * Print out log information in kernel.
 */

#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>

#include "percpu.h"
#include "printk.h"
#include "syscall.h"
#include "kernel/sched.h"

#define MAX_LOG_LEN	1024

/**
 * printk - print out information on specific core
*/
int printk(const char *fmt, ...) {
    va_list ptr;
	char buf[MAX_LOG_LEN];
	time_t ts;
	off_t off = 0;

	if (!kernel_early_boot) {
        snprintf(buf, 22, "CPU %02d| PID %8u ", cpu_id, getpid());
		off = strlen(buf);
    }

    time(&ts);
	off += strftime(buf + off, 32, "%H:%M:%S ", localtime(&ts));

	va_start(ptr, fmt);
	vsnprintf(buf + off, MAX_LOG_LEN - off, fmt, ptr);
	va_end(ptr);

	printf("%s", buf);

	return 0;
}
