#ifndef _PTRACE_H_
#define _PTRACE_H_

#include <stdint.h>

struct pt_regs {
/*
 * C ABI says these regs are callee-preserved. They aren't saved on kernel entry
 * unless syscall needs a complete, fully filled "struct pt_regs".
 */
	uint64_t 	r15;
	uint64_t 	r14;
	uint64_t 	r13;
	uint64_t 	r12;
	uint64_t 	bp;
	uint64_t 	bx;
/* These regs are callee-clobbered. Always saved on kernel entry. */
	uint64_t 	r11;
	uint64_t 	r10;
	uint64_t 	r9;
	uint64_t 	r8;
	uint64_t 	ax;
	uint64_t 	cx;
	uint64_t 	dx;
	uint64_t 	si;
	uint64_t 	di;
/*
 * On syscall entry, this is syscall#. On CPU exception, this is error code.
 * On hw interrupt, it's IRQ number:
 */
	uint64_t 	orig_ax;
/* Return frame */
	uint64_t 	eflags;
	uint64_t 	sp;
    uint64_t	ip;
/* top of stack page */
};

#endif	/* _PTRACE_H_ */