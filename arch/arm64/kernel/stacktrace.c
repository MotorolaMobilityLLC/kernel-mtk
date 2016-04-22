/*
 * Stack tracing support
 *
 * Copyright (C) 2012 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/sched.h>
#include <linux/stacktrace.h>

#include <asm/irq.h>
#include <asm/stacktrace.h>

/*
 * AArch64 PCS assigns the frame pointer to x29.
 *
 * A simple function prologue looks like this:
 * 	sub	sp, sp, #0x10
 *   	stp	x29, x30, [sp]
 *	mov	x29, sp
 *
 * A simple function epilogue looks like this:
 *	mov	sp, x29
 *	ldp	x29, x30, [sp]
 *	add	sp, sp, #0x10
 */
int notrace unwind_frame(struct stackframe *frame)
{
	unsigned long high, low;
	unsigned long fp = frame->fp;
#ifdef CONFIG_ARM64_IRQ_STACK
	unsigned long _irq_stack_ptr;

	/*
	 * Use raw_smp_processor_id() to avoid false-positives from
	 * CONFIG_DEBUG_PREEMPT. get_wchan() calls unwind_frame() on sleeping
	 * task stacks, we can be pre-empted in this case, so
	 * {raw_,}smp_processor_id() may give us the wrong value. Sleeping
	 * tasks can't ever be on an interrupt stack, so regardless of cpu,
	 * the checks will always fail.
	 */
	_irq_stack_ptr = per_cpu(irq_stack_ptr, raw_smp_processor_id());
#endif
	low  = frame->sp;
#ifndef CONFIG_ARM64_IRQ_STACK
	high = ALIGN(low, THREAD_SIZE);
#else
	/* irq stacks are not THREAD_SIZE aligned */
	if (on_irq_stack(frame->sp, raw_smp_processor_id()))
		high = _irq_stack_ptr;
	else
		high = ALIGN(low, THREAD_SIZE) - 0x20;
#endif
#ifndef CONFIG_ARM64_IRQ_STACK
	if (fp < low || fp > high - 0x18 || fp & 0xf)
#else
	if (fp < low || fp > high || fp & 0xf)
#endif
		return -EINVAL;

	frame->sp = fp + 0x10;
	frame->fp = *(unsigned long *)(fp);
#ifdef CONFIG_ARM64_IRQ_STACK
	frame->pc = *(unsigned long *)(fp + 8);
#else
	/*
	 * -4 here because we care about the PC at time of bl,
	 * not where the return will go.
	 */
	frame->pc = *(unsigned long *)(fp + 8) - 4;
#endif

#ifdef CONFIG_ARM64_IRQ_STACK
	/*
	 * Check whether we are going to walk through from interrupt stack
	 * to task stack.
	 * If we reach the end of the stack - and its an interrupt stack,
	 * read the original task stack pointer from the dummy frame.
	 */
	if (frame->sp == _irq_stack_ptr)
		frame->sp = IRQ_STACK_TO_TASK_STACK(_irq_stack_ptr);
#endif
	return 0;
}

void notrace walk_stackframe(struct stackframe *frame,
		     int (*fn)(struct stackframe *, void *), void *data)
{
	while (1) {
		int ret;

		if (fn(frame, data))
			break;
		ret = unwind_frame(frame);
		if (ret < 0)
			break;
	}
}
EXPORT_SYMBOL(walk_stackframe);

#ifdef CONFIG_STACKTRACE
struct stack_trace_data {
	struct stack_trace *trace;
	unsigned int no_sched_functions;
	unsigned int skip;
};

static int save_trace(struct stackframe *frame, void *d)
{
	struct stack_trace_data *data = d;
	struct stack_trace *trace = data->trace;
	unsigned long addr = frame->pc;

	if (data->no_sched_functions && in_sched_functions(addr))
		return 0;
	if (data->skip) {
		data->skip--;
		return 0;
	}

	trace->entries[trace->nr_entries++] = addr;

	return trace->nr_entries >= trace->max_entries;
}

void save_stack_trace_tsk(struct task_struct *tsk, struct stack_trace *trace)
{
	struct stack_trace_data data;
	struct stackframe frame;

	data.trace = trace;
	data.skip = trace->skip;

	if (tsk != current) {
		data.no_sched_functions = 1;
		frame.fp = thread_saved_fp(tsk);
		frame.sp = thread_saved_sp(tsk);
		frame.pc = thread_saved_pc(tsk);
	} else {
		data.no_sched_functions = 0;
		frame.fp = (unsigned long)__builtin_frame_address(0);
		frame.sp = current_stack_pointer;
		frame.pc = (unsigned long)save_stack_trace_tsk;
	}

	walk_stackframe(&frame, save_trace, &data);
	if (trace->nr_entries < trace->max_entries)
		trace->entries[trace->nr_entries++] = ULONG_MAX;
}

void save_stack_trace(struct stack_trace *trace)
{
	save_stack_trace_tsk(current, trace);
}
EXPORT_SYMBOL_GPL(save_stack_trace);
#endif
