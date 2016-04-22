#ifndef __ASM_IRQ_H
#define __ASM_IRQ_H

#include <linux/percpu.h>

#include <asm-generic/irq.h>
#include <asm/thread_info.h>

#ifdef CONFIG_ARM64_IRQ_STACK
#define __ARCH_HAS_DO_SOFTIRQ

DECLARE_PER_CPU(unsigned long, irq_stack_ptr);

#define IRQ_STACK_SIZE			THREAD_SIZE
#define IRQ_STACK_START_SP		THREAD_START_SP

/*
 * This is the offset from irq_stack_ptr where entry.S will store the original
 * stack pointer. Used by unwind_frame() and dump_backtrace().
 */
#define IRQ_STACK_TO_TASK_STACK(x)	(*((unsigned long *)(x - 0x20)))
#endif

extern void migrate_irqs(void);
extern void set_handle_irq(void (*handle_irq)(struct pt_regs *));

#ifdef CONFIG_ARM64_IRQ_STACK
void init_irq_stack(unsigned int cpu);

static inline bool on_irq_stack(unsigned long sp, int cpu)
{
	/* variable names the same as kernel/stacktrace.c */
	unsigned long high = per_cpu(irq_stack_ptr, cpu);
	unsigned long low = high - IRQ_STACK_START_SP;

	return (low <= sp && sp <= high);
}
#endif
#endif
