#include <linux/bug.h>
#include <linux/kallsyms.h>
#include <linux/ptrace.h>
#include <asm/stacktrace.h>
#include <asm/system_misc.h>
#include <asm/traps.h>
#include "mrdump_private.h"

static void mrdump_dump_backtrace(struct pt_regs *regs)
{
	struct stackframe frame;
	int count = 0;

	if (regs == NULL)
		return;

	frame.fp = regs->regs[29];
	frame.sp = regs->sp;
	frame.pc = regs->pc;
	pr_notice("Call trace:\n");
	while (count++ < 32) {
		unsigned long where = frame.pc;
		int ret;

		ret = unwind_frame(&frame);
		if (ret < 0)
			break;
		print_ip_sym(where);
#if 0
		if (in_exception_text(where))
			dump_mem("", "Exception stack", frame.sp,
				 frame.sp + sizeof(struct pt_regs));
#endif
	}
}

void mrdump_save_current_backtrace(struct pt_regs *regs)
{
	asm volatile ("stp x0, x1, [%0]\n\t"
		      "stp x2, x3, [%0, #16]\n\t"
		      "stp x4, x5, [%0, #32]\n\t"
		      "stp x6, x7, [%0, #48]\n\t"
		      "stp x8, x9, [%0, #64]\n\t"
		      "stp x10, x11, [%0, #80]\n\t"
		      "stp x12, x13, [%0, #96]\n\t"
		      "stp x14, x15, [%0, #112]\n\t"
		      "stp x16, x17, [%0, #128]\n\t"
		      "stp x18, x19, [%0, #144]\n\t"
		      "stp x20, x21, [%0, #160]\n\t"
		      "stp x22, x23, [%0, #176]\n\t"
		      "stp x24, x25, [%0, #192]\n\t"
		      "stp x26, x27, [%0, #208]\n\t"
		      "stp x28, x29, [%0, #224]\n\t"
		      "str x30, [%0, #240]\n\t" : : "r" (&regs->user_regs) : "memory");
	asm volatile ("mov x8, sp\n\t"
		      "str x8, [%0]\n\t"
		      "1:\n\t"
		      "adr x8, 1b\n\t"
		      "str x8, [%1]\n\t" : : "r" (&regs->user_regs.sp), "r"(&regs->user_regs.pc) : "x8", "memory");
}

void mrdump_print_crash(struct pt_regs *regs)
{
	__show_regs(regs);
	mrdump_dump_backtrace(regs);
}
