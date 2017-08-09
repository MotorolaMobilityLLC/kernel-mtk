/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <mt-plat/mt_boot.h>
#ifdef CONFIG_SMP
#include <linux/cpu.h>
#endif
#include "hw_watchpoint_aarch64.h"
#include "mt_dbg_aarch64.h"

struct dbgreg_set dbgregs[8];

#ifdef DBG_REG_DUMP
void dump_dbgregs(int cpuid)
{
	struct wp_trace_context_t *wp_context;
	int i;

	register_wp_context(&wp_context);
	cs_cpu_write(wp_context->debug_regs[cpuid], EDLAR, UNLOCK_KEY);
	cs_cpu_write(wp_context->debug_regs[cpuid], OSLAR_EL1, ~UNLOCK_KEY);
	isb();

	smp_call_function_single(cpuid, smp_read_MDSCR_EL1_callback, &dbgregs[cpuid].MDSCR_EL1, 1);

	for (i = 1; i < 1 + (wp_context->bp_nr); i++) {
		dbgregs[cpuid].regs[i] =
		    cs_cpu_read_64(wp_context->debug_regs[cpuid], (DBGBVR + ((i - 1) << 4)));
	}

	for (i = 7; i < 7 + (wp_context->bp_nr); i++) {
		dbgregs[cpuid].regs[i] =
		    cs_cpu_read(wp_context->debug_regs[cpuid], (DBGBCR + ((i - 7) << 4)));
	}

	for (i = 13; i < 13 + (wp_context->wp_nr); i++) {
		dbgregs[cpuid].regs[i] =
		    cs_cpu_read_64(wp_context->debug_regs[cpuid], (DBGWVR + ((i - 13) << 4)));
	}

	for (i = 17; i < 17 + (wp_context->wp_nr); i++) {
		dbgregs[cpuid].regs[i] =
		    cs_cpu_read(wp_context->debug_regs[cpuid], (DBGWCR + ((i - 17) << 4)));
	}


	isb();

}

void print_dbgregs(int cpuid)
{

	pr_debug("[MTK WP] cpu %d, MDSCR_EL1 0x%lx\n", cpuid, dbgregs[cpuid].MDSCR_EL1);
	pr_debug("[MTK WP] cpu %d, DBGBVR0 0x%lx\n", cpuid, dbgregs[cpuid].DBGBVR0);
	pr_debug("[MTK WP] cpu %d, DBGBVR1 0x%lx\n", cpuid, dbgregs[cpuid].DBGBVR1);
	pr_debug("[MTK WP] cpu %d, DBGBVR2 0x%lx\n", cpuid, dbgregs[cpuid].DBGBVR2);
	pr_debug("[MTK WP] cpu %d, DBGBVR3 0x%lx\n", cpuid, dbgregs[cpuid].DBGBVR3);
	pr_debug("[MTK WP] cpu %d, DBGBVR4 0x%lx\n", cpuid, dbgregs[cpuid].DBGBVR4);
	pr_debug("[MTK WP] cpu %d, DBGBVR5 0x%lx\n", cpuid, dbgregs[cpuid].DBGBVR5);

	pr_debug("[MTK WP] cpu %d, DBGBCR0 0x%lx\n", cpuid, dbgregs[cpuid].DBGBCR0);
	pr_debug("[MTK WP] cpu %d, DBGBCR1 0x%lx\n", cpuid, dbgregs[cpuid].DBGBCR1);
	pr_debug("[MTK WP] cpu %d, DBGBCR2 0x%lx\n", cpuid, dbgregs[cpuid].DBGBCR2);
	pr_debug("[MTK WP] cpu %d, DBGBCR3 0x%lx\n", cpuid, dbgregs[cpuid].DBGBCR3);
	pr_debug("[MTK WP] cpu %d, DBGBCR4 0x%lx\n", cpuid, dbgregs[cpuid].DBGBCR4);
	pr_debug("[MTK WP] cpu %d, DBGBCR5 0x%lx\n", cpuid, dbgregs[cpuid].DBGBCR5);


	pr_debug("[MTK WP] cpu %d, DBGWVR0 0x%lx\n", cpuid, dbgregs[cpuid].DBGWVR0);
	pr_debug("[MTK WP] cpu %d, DBGWVR1 0x%lx\n", cpuid, dbgregs[cpuid].DBGWVR1);
	pr_debug("[MTK WP] cpu %d, DBGWVR2 0x%lx\n", cpuid, dbgregs[cpuid].DBGWVR2);
	pr_debug("[MTK WP] cpu %d, DBGWVR3 0x%lx\n", cpuid, dbgregs[cpuid].DBGWVR3);

	pr_debug("[MTK WP] cpu %d, DBGWCR0 0x%lx\n", cpuid, dbgregs[cpuid].DBGWCR0);
	pr_debug("[MTK WP] cpu %d, DBGWCR1 0x%lx\n", cpuid, dbgregs[cpuid].DBGWCR1);
	pr_debug("[MTK WP] cpu %d, DBGWCR2 0x%lx\n", cpuid, dbgregs[cpuid].DBGWCR2);
	pr_debug("[MTK WP] cpu %d, DBGWCR3 0x%lx\n", cpuid, dbgregs[cpuid].DBGWCR3);

	/*
		pr_debug("[MTK WP] cpu %d, DBGVCR 0x%lx\n",cpuid,dbgregs[cpuid].DBGVCR);
		pr_debug("[MTK WP] cpu %d, SDSR 0x%lx\n",cpuid,dbgregs[cpuid].SDSR);
}	*/

#endif
#if 1
unsigned int *mt_save_dbg_regs(unsigned int *p, unsigned int cpuid)
{

#if 0
	struct wp_trace_context_t *wp_context;

	register_wp_context(&wp_context);
	cs_cpu_write(wp_context->debug_regs[cpuid], EDLAR, UNLOCK_KEY);
	cs_cpu_write(wp_context->debug_regs[cpuid], OSLAR_EL1, ~UNLOCK_KEY);
#ifdef DBG_REG_DUMP
	pr_debug("[MTK WP] %s\n", __func__);
#endif
	if (*p == ~0x0) {
		pr_err("[MTK WP]restore pointer is NULL\n");
		return 0;
	}


	isb();
	/* save register */

	__asm__ __volatile__("mrs x0, MDSCR_EL1\n\t"
			     "str x0, [%0],#0x8\n\t"
			     "mrs x0, DBGBVR0_EL1\n\t"
			     "mrs x1, DBGBVR1_EL1\n\t"
			     "mrs x2, DBGBVR2_EL1\n\t"
			     "mrs x3, DBGBVR3_EL1\n\t"
			     "mrs x4, DBGBVR4_EL1\n\t"
			     "mrs x5, DBGBVR5_EL1\n\t"
			     "str x0 , [%0],#0x8\n\t"
			     "str x1 , [%0],#0x8\n\t"
			     "str x2 , [%0],#0x8\n\t"
			     "str x3 , [%0],#0x8\n\t"
			     "str x4 , [%0],#0x8\n\t"
			     "str x5 , [%0],#0x8\n\t"
			     "mrs x0, DBGBCR0_EL1\n\t"
			     "mrs x1, DBGBCR1_EL1\n\t"
			     "mrs x2, DBGBCR2_EL1\n\t"
			     "mrs x3, DBGBCR3_EL1\n\t"
			     "mrs x4, DBGBCR4_EL1\n\t"
			     "mrs x5, DBGBCR5_EL1\n\t"
			     "str x0 , [%0],#0x8\n\t"
			     "str x1 , [%0],#0x8\n\t"
			     "str x2 , [%0],#0x8\n\t"
			     "str x3 , [%0],#0x8\n\t"
			     "str x4 , [%0],#0x8\n\t"
			     "str x5 , [%0],#0x8\n\t"
			     "mrs x0, DBGWVR0_EL1\n\t"
			     "mrs x1, DBGWVR1_EL1\n\t"
			     "mrs x2, DBGWVR2_EL1\n\t"
			     "mrs x3, DBGWVR3_EL1\n\t"
			     "str x0 , [%0],#0x8\n\t"
			     "str x1 , [%0],#0x8\n\t"
			     "str x2 , [%0],#0x8\n\t"
			     "str x3 , [%0],#0x8\n\t"
			     "mrs x0, DBGWCR0_EL1\n\t"
			     "mrs x1, DBGWCR1_EL1\n\t"
			     "mrs x2, DBGWCR2_EL1\n\t"
			     "mrs x3, DBGWCR3_EL1\n\t"
			     "str x0 , [%0],#0x8\n\t"
			     "str x1 , [%0],#0x8\n\t"
			     "str x2 , [%0],#0x8\n\t" "str x3 , [%0],#0x8\n\t":"+r"(p)
			     : : "x0", "x1", "x2", "x3", "x4", "x5");
	isb();

#endif
	return p;
}

void mt_restore_dbg_regs(unsigned int *p, unsigned int cpuid)
{
#if 0
	unsigned long dscr;
	struct wp_trace_context_t *wp_context;
#ifdef DBG_REG_DUMP
	pr_debug("[MTK WP] %s\n", __func__);
#endif
	register_wp_context(&wp_context);
	/* the dbg container is invalid */
	if (*p == ~0x0) {
		pr_err("[MTK WP]restore pointer is NULL\n");
		return;
	}
	cs_cpu_write(wp_context->debug_regs[cpuid], EDLAR, UNLOCK_KEY);
	cs_cpu_write(wp_context->debug_regs[cpuid], OSLAR_EL1, ~UNLOCK_KEY);

	isb();


	/* restore register */
#if 1
	__asm__ __volatile__("ldr x0, [%0],#0x8\n\t"
			     "mov %1, x0\n\t"
			     "msr MDSCR_EL1,x0\n\t"
			     "ldr x0 , [%0],#0x8\n\t"
			     "ldr x1 , [%0],#0x8\n\t"
			     "ldr x2 , [%0],#0x8\n\t"
			     "ldr x3 , [%0],#0x8\n\t"
			     "ldr x4 , [%0],#0x8\n\t"
			     "ldr x5 , [%0],#0x8\n\t"
			     "msr DBGBVR0_EL1,x0\n\t"
			     "msr DBGBVR1_EL1,x1\n\t"
			     "msr DBGBVR2_EL1,x2\n\t"
			     "msr DBGBVR3_EL1,x3\n\t"
			     "msr DBGBVR4_EL1,x4\n\t"
			     "msr DBGBVR5_EL1,x5\n\t"
			     "ldr x0 , [%0],#0x8\n\t"
			     "ldr x1 , [%0],#0x8\n\t"
			     "ldr x2 , [%0],#0x8\n\t"
			     "ldr x3 , [%0],#0x8\n\t"
			     "ldr x4 , [%0],#0x8\n\t"
			     "ldr x5 , [%0],#0x8\n\t"
			     "msr DBGBCR0_EL1,x0\n\t"
			     "msr DBGBCR1_EL1,x1\n\t"
			     "msr DBGBCR2_EL1,x2\n\t"
			     "msr DBGBCR3_EL1,x3\n\t"
			     "msr DBGBCR4_EL1,x4\n\t"
			     "msr DBGBCR5_EL1,x5\n\t"
			     "ldr x0 , [%0],#0x8\n\t"
			     "ldr x1 , [%0],#0x8\n\t"
			     "ldr x2 , [%0],#0x8\n\t"
			     "ldr x3 , [%0],#0x8\n\t"
			     "msr DBGWVR0_EL1,x0\n\t"
			     "msr DBGWVR1_EL1,x1\n\t"
			     "msr DBGWVR2_EL1,x2\n\t"
			     "msr DBGWVR3_EL1,x3\n\t"
			     "ldr x0 , [%0],#0x8\n\t"
			     "ldr x1 , [%0],#0x8\n\t"
			     "ldr x2 , [%0],#0x8\n\t"
			     "ldr x3 , [%0],#0x8\n\t"
			     "msr DBGWCR0_EL1,x0\n\t"
			     "msr DBGWCR1_EL1,x1\n\t"
			     "msr DBGWCR2_EL1,x2\n\t"
			     "msr DBGWCR3_EL1,x3\n\t":"+r"(p), "=r"(dscr)
			     : : "x0", "x1", "x2", "x3", "x4", "x5");
#endif
	isb();
#endif
}





/** to copy dbg registers from FROM to TO within the same cluster.
 * DBG_BASE is the common base of 2 cores debug register space.
 **/
void mt_copy_dbg_regs(int to, int from)
{
#if 0
	unsigned long base_to, base_from;
	unsigned long args;
	struct wp_trace_context_t *wp_context;

	register_wp_context(&wp_context);
	base_to = (unsigned long)wp_context->debug_regs[to];
	base_from = (unsigned long)wp_context->debug_regs[from];

	/* os unlock */
	cs_cpu_write(wp_context->debug_regs[to], EDLAR, UNLOCK_KEY);
	cs_cpu_write(wp_context->debug_regs[to], OSLAR_EL1, ~UNLOCK_KEY);
	cs_cpu_write(wp_context->debug_regs[from], EDLAR, UNLOCK_KEY);
	cs_cpu_write(wp_context->debug_regs[from], OSLAR_EL1, ~UNLOCK_KEY);

	isb();

	smp_call_function_single(from, smp_read_MDSCR_EL1_callback, &args, 1);
	smp_call_function_single(to, smp_write_MDSCR_EL1_callback, &args, 1);
	isb();
	dbg_reg_copy(DBGBCR, base_to, base_from);
	dbg_reg_copy(DBGBCR + 0x10, base_to, base_from);
	dbg_reg_copy(DBGBCR + 0x20, base_to, base_from);
	dbg_reg_copy(DBGBCR + 0x30, base_to, base_from);
	dbg_reg_copy(DBGBCR + 0x40, base_to, base_from);
	dbg_reg_copy(DBGBCR + 0x50, base_to, base_from);

	dbg_reg_copy_64(DBGBVR, base_to, base_from);
	dbg_reg_copy_64(DBGBVR + 0x10, base_to, base_from);
	dbg_reg_copy_64(DBGBVR + 0x20, base_to, base_from);
	dbg_reg_copy_64(DBGBVR + 0x30, base_to, base_from);
	dbg_reg_copy_64(DBGBVR + 0x40, base_to, base_from);
	dbg_reg_copy_64(DBGBVR + 0x50, base_to, base_from);


	dbg_reg_copy_64(DBGWVR, base_to, base_from);
	dbg_reg_copy_64(DBGWVR + 0x10, base_to, base_from);
	dbg_reg_copy_64(DBGWVR + 0x20, base_to, base_from);
	dbg_reg_copy_64(DBGWVR + 0x30, base_to, base_from);

	dbg_reg_copy(DBGWCR, base_to, base_from);
	dbg_reg_copy(DBGWCR + 0x10, base_to, base_from);
	dbg_reg_copy(DBGWCR + 0x20, base_to, base_from);
	dbg_reg_copy(DBGWCR + 0x30, base_to, base_from);

	isb();
#endif
}



#ifdef CONFIG_SMP
int __cpuinit dbgregs_hotplug_callback(struct notifier_block *nfb, unsigned long action, void *hcpu)
{
	unsigned long this_cpu = (unsigned long)hcpu;
	unsigned int args = 0;
	struct wp_trace_context_t *wp_context;
	unsigned long base_to, base_from;

	register_wp_context(&wp_context);
	cs_cpu_write(wp_context->debug_regs[this_cpu], EDLAR, UNLOCK_KEY);
	cs_cpu_write(wp_context->debug_regs[this_cpu], OSLAR_EL1, ~UNLOCK_KEY);
	cs_cpu_write(wp_context->debug_regs[0], EDLAR, UNLOCK_KEY);
	cs_cpu_write(wp_context->debug_regs[0], OSLAR_EL1, ~UNLOCK_KEY);
	base_to = (unsigned long)wp_context->debug_regs[this_cpu];
	base_from = (unsigned long)wp_context->debug_regs[0];
	action = action & 0xf;
#ifdef DBG_REG_DUMP
	pr_debug("[MTK WP] cpu %lx do %s,action: 0x%lx\n", this_cpu, __func__, action);
#endif
	switch (action) {
	case CPU_STARTING:
		smp_call_function_single(0, smp_read_MDSCR_EL1_callback, &args, 1);
#ifdef DBG_REG_DUMP
		pr_debug("[MTK WP] cpu %lx do %s, CPU0's _MDSCR_EL1=0x%x\n", this_cpu, __func__,
			  args);
#endif
		asm volatile ("msr  MDSCR_EL1, %0" : : "r" (args));
		isb();
		dbg_reg_copy(DBGBCR, base_to, base_from);
		dbg_reg_copy(DBGBCR + 0x10, base_to, base_from);
		dbg_reg_copy(DBGBCR + 0x20, base_to, base_from);
		dbg_reg_copy(DBGBCR + 0x30, base_to, base_from);
		dbg_reg_copy(DBGBCR + 0x40, base_to, base_from);
		dbg_reg_copy(DBGBCR + 0x50, base_to, base_from);

		dbg_reg_copy_64(DBGBVR, base_to, base_from);
		dbg_reg_copy_64(DBGBVR + 0x10, base_to, base_from);
		dbg_reg_copy_64(DBGBVR + 0x20, base_to, base_from);
		dbg_reg_copy_64(DBGBVR + 0x30, base_to, base_from);
		dbg_reg_copy_64(DBGBVR + 0x40, base_to, base_from);
		dbg_reg_copy_64(DBGBVR + 0x50, base_to, base_from);


		dbg_reg_copy_64(DBGWVR, base_to, base_from);
		dbg_reg_copy_64(DBGWVR + 0x10, base_to, base_from);
		dbg_reg_copy_64(DBGWVR + 0x20, base_to, base_from);
		dbg_reg_copy_64(DBGWVR + 0x30, base_to, base_from);

		dbg_reg_copy(DBGWCR, base_to, base_from);
		dbg_reg_copy(DBGWCR + 0x10, base_to, base_from);
		dbg_reg_copy(DBGWCR + 0x20, base_to, base_from);
		dbg_reg_copy(DBGWCR + 0x30, base_to, base_from);

		isb();
		break;

	default:
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block cpu_nfb __cpuinitdata = {
	.notifier_call = dbgregs_hotplug_callback
};

static int __init regs_backup(void)
{
	return 0;

	register_cpu_notifier(&cpu_nfb);

	return 0;
}

module_init(regs_backup);
#endif
#endif
