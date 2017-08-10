/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
#include <linux/types.h>
#include <linux/sizes.h>
#include <linux/printk.h>
#include <linux/mm.h>
#include <linux/compat.h>
#include <linux/init.h>
#include <asm/traps.h>
#include <linux/ptrace.h>
#include <asm/cacheflush.h>
#include <asm/pgtable.h>
#include <linux/atomic.h>
#include <mt-plat/mt_cache_dump.h>

static atomic_t reg_0x1001Axxx_cnt = ATOMIC_INIT(1);

void ioremap_debug_hook_func(phys_addr_t phys_addr, size_t size, pgprot_t prot)
{
	unsigned long offset = phys_addr & ~PAGE_MASK;
	phys_addr_t target = 0x1001A000;

	target &= PAGE_MASK;
	phys_addr &= PAGE_MASK;
	size = PAGE_ALIGN(size + offset);

	/* only allow 1st ioremap to address 0x1001AXXX */
	if ((phys_addr + size <= target) ||
			(phys_addr >= (target + PAGE_SIZE)))
		return;

	if (atomic_dec_and_test(&reg_0x1001Axxx_cnt))
		return;

	/* trap other ioremap to 0x1001AXXX */
	pr_err("ioremap to 0x1001AXXX more than once (%pa, %zx)\n",
			&phys_addr, size);
	BUG();
}

static DEFINE_PER_CPU(void *, __prev_undefinstr_pc);
static DEFINE_PER_CPU(int, __prev_undefinstr_counter);

/*
 * return 0 if code is fixed
 * return 1 if the undefined instruction cannot be fixed.
 */
int arm_undefinstr_retry(struct pt_regs *regs, unsigned int instr)
{
	void __user *pc = (void __user *)instruction_pointer(regs);
	struct thread_info *thread = current_thread_info();

	mt_icache_dump();

	/*
	 * Place the SIGILL ICache Invalidate after the Debugger
	 * Undefined-Instruction Solution.
	 */
	if ((user_mode(regs)) || processor_mode(regs) == PSR_MODE_EL1h) {
		void **prev_undefinstr_pc = &get_cpu_var(__prev_undefinstr_pc);
		int *prev_undefinstr_counter = &get_cpu_var(__prev_undefinstr_counter);

		pr_alert("USR_MODE / SVC_MODE Undefined Instruction Address curr:%p pc=%p:%p, instr: 0x%x compat: %s\n",
			(void *)current, (void *)pc, (void *)*prev_undefinstr_pc, instr,
			is_compat_task() ? "yes" : "no");
		if (*prev_undefinstr_pc != pc) {
			/*
			 * If the current process or program counter is changed...
			 * renew the counter.
			 */
			pr_alert("First Time Recovery curr:%p pc=%p:%p\n",
				(void *)current, (void *)pc, (void *)*prev_undefinstr_pc);
			*prev_undefinstr_pc = pc;
			*prev_undefinstr_counter = 0;
			put_cpu_var(__prev_undefinstr_pc);
			put_cpu_var(__prev_undefinstr_counter);
			__flush_icache_all();
			__inner_flush_dcache_all();
			/*
			 * undo cpu_excp to cancel nest_panic code, see entry.S
			 */
			if (!user_mode(regs))
				thread->cpu_excp--;
			return 0;
		} else if (*prev_undefinstr_counter < 2) {
			pr_alert("%dth Time Recovery curr:%p pc=%p:%p\n",
				*prev_undefinstr_counter + 1,
				(void *)current, (void *)pc,
				(void *)*prev_undefinstr_pc);
			*prev_undefinstr_counter += 1;
			put_cpu_var(__prev_undefinstr_pc);
			put_cpu_var(__prev_undefinstr_counter);
			__flush_icache_all();
			__inner_flush_dcache_all();
			/*
			 * undo cpu_excp to cancel nest_panic code, see entry.S
			 */
			if (!user_mode(regs))
				thread->cpu_excp--;
			return 0;
		}
		*prev_undefinstr_counter += 1;

		if (*prev_undefinstr_counter >= 2) {
			*prev_undefinstr_pc = 0;
			*prev_undefinstr_counter = 0;
		}
		put_cpu_var(__prev_undefinstr_pc);
		put_cpu_var(__prev_undefinstr_counter);
		pr_alert("Go to ARM Notify Die curr:%p pc=%p:%p\n",
			(void *)current, (void *)pc, (void *)*prev_undefinstr_pc);
	}
	return 1;
}
